//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>//Para el open
#include <sys/types.h> //Para Socket
#include <sys/socket.h> //Para Socket
#include <arpa/inet.h> //Para Socket
#include <errno.h> //Para perror
#include <pthread.h> //Para Hilos
#include <string.h> //Para memset
#include <mqueue.h> //Para la cola de mensajes
#include <jansson.h> //Para parsear json
#include <xbee.h> // Librería para manejo interno de Xbees

#include "utilidades.h" //Funciones propias
#include "http.h" //Funciones propias
#include "xbee.h" //Funciones propias
#include "hilos.h" //Funciones propias


//-----------------------------------
//-- VARIABLESGLOBALES Y ESTRUCTURAS
//-----------------------------------

char *dirbeeboxmqsend; //Ruta donde está el ejecutable que escribirá las tareas en la cola (para cron) (se setea en el archivo de configuración)

//------------------
//-- FUNCIONES
//------------------

int main(int argc, char *const argv[]){

	int descsocket; //Descriptor del Socket (descriptor pasivo, tiene solo dos valores [IP y PUERTO local])
	int newdescsocket; //Descriptor para las conecciones (descriptor activo, tiene los 4 valores [IP LOCAL Y de CLIENTE, PUERTO LOCAL Y de CLIENTE])
	struct sockaddr_in midireccion = {};//Estructura en donde estan los datos del socket (como ip y puerto)
	struct sockaddr_in6 midireccion6 = {};//Estructura en donde estan los datos del socket (como ip y puerto) para ipv6
	int reuse = 1;//Para setsockopt. Indica el tiempo en el que se podrá volver a iniciar el servidor en caso de cerrarse
	int ipv6 = 0;//Si están en 1 ek socket se crean en ipv6 (seteado en opciones)

	char *ipserver;//IP del servidor (se setea en el archivo de configuración)
	int puerto;//Puerto del servidor (se setea en el archivo de configuración)

	char *puertoserie; //Puerto serie al que está conectado el xbee coordinador (se setea en el archivo de configuración)
	char *baudiosstring; //Velocidad en baudios a los que se establecerá la conexión el xbee coordinador (se setea en el archivo de configuración)
	int baudios;

	char *dirraiz;//Directorio raiz para obtener archivos solicitados al servidor (se setea en el archivo de configuración)
	struct xbee *xbee; //Descriptor a xbee conectado al servidor
	xbee_err err; //Variable para guardar errores

	pthread_t idhilo; //Esta variable contendrá el id del hilo creado
	int opcion; //Para el manejo de argumentos
	int leido;
	char buffer[1024];
	char *archconfig = "beebox.cfg";//Puntero al nombre del archivo de configuración. Por defecto es server.cfg
	int acd; //Descriptor del archivo de configuracion
	char *puertostring;//Para luego convertirlo a int

	arg_struct argumentos;//Estructura para mandar argumentos a los hilos nuevos
	arg_struct *ptrarg;//Puntero a estructura de argumentos

	//Se analizan los argumentos y se carga en las variables nhijos y archivosalida lo que el usuario coloque en los argumentos -n y -o al iniciar el programa

	while ((opcion = getopt (argc, argv, "f:6")) != -1){
		switch (opcion){
			case 'f':
				archconfig = optarg; //Si se especifica un archivo de configuracion con el argumento -f se modifica la variable archconfig apuntando al nuevo archivo de configuracion
				break;
			case '6':
				ipv6 = 1; //Si se especifica el argumento -6 se modifica la variable ipv6 a 1 (el server será ipv6)
				break;
			case '?':
				printf ("Argumentos Incorrectos");
				return 1;
			default:
				printf ("Argumentos Incorrectos");
				return 1;
		}
	}

	//Leemos el archivo de configuracion y guardamos sus datos en las variables globales correspondientes
	if ((acd = open(archconfig,O_RDONLY,0)) < 0){
		printf("Error al abrir el archivo de configuración especificado");
		return 1;
	}
	leido = read (acd, buffer, sizeof buffer);
	buffer[leido] = '\0'; //Agrega un \0 al final de lo que se guardó en el buffer así strtok trabaja correctamente
	//Parseamos para dejar en la variable ipserver la ip especificado en el archivo de configuracion
	ipserver = strtok (buffer, "=");
	ipserver = strtok (NULL, "\n");
	printf ("IP Server: %s\n", ipserver);
	//Parseamos para dejar en la variable puerto el puerto especificado en el archivo de configuracion (pero en string)
	puertostring = strtok (NULL, "=");
	puertostring = strtok (NULL, "\n");
	puerto = atoi(puertostring);
	printf ("Puerto Server: %d\n", puerto);
	//Parseamos para dejar en la variable dirraiz el directorio raiz especificado en el archivo de configuracion
	dirraiz = strtok (NULL, "=");
	dirraiz = strtok (NULL, "\n");
	printf ("Directorio Raiz: %s\n", dirraiz);
	//Parseamos para dejar en la variable puertoserie el puerto especificado en el archivo de configuracion (string)
	puertoserie = strtok (NULL, "=");
	puertoserie = strtok (NULL, "\n");
	printf ("Puerto Xbee Coordinador: %s\n", puertoserie);
	//Parseamos para dejar en la variable baudiosstring los baudios especificados en el archivo de configuracion (en string)
	baudiosstring = strtok (NULL, "=");
	baudiosstring = strtok (NULL, "\n");
	baudios = atoi(baudiosstring); //transformamos el string a int y lo guardamos en la variable baudios
	printf ("Baudios Coordinador: %d\n", baudios);
	//Parseamos para dejar en la variable dirbeeboxmqsend la ruta dónde está dicho ejecutable
	dirbeeboxmqsend = strtok (NULL, "=");
	dirbeeboxmqsend = strtok (NULL, "\n");
	printf ("Ejecutable beeboxmqsend Raiz: %s\n", dirbeeboxmqsend);


	// //Setea la conexión con el xbee coordinador por puerto serie
	// if ((err = xbee_setup(&xbee, "xbeeZB", puertoserie, baudios)) != XBEE_ENONE) {
	// 	fprintf(stderr, "ERROR AL INTENTAR ABRIR CONEXIÓN POR PUERTO SERIE: %d (%s)\n", err, xbee_errorToStr(err));
	// 	return err;
	// }
	// printf ("Conexión con coordinador establecida en %s a %d baudios.\n", puertoserie, baudios);


	//Si se seteó la opciónde ipv6 se abre un socket ipv6, si no uno ipv4
	if (ipv6 == 1){
		printf("Creando socket IPv6...\n");
		//Creamos el SOCKET
		descsocket = socket(PF_INET6, SOCK_STREAM, 0); //Creo el socket del tipo PF_INET (Internet), SOCK_STREAM (TCP)
		if (descsocket < 0){
			perror("No se creo el socket");
			return -1;
		}
		//Modificamos las opciones del socket para que podamos iniciarlo nuevamente sin esperar en el caso que se cierre
		setsockopt(descsocket, SOL_SOCKET, SO_REUSEADDR,  (char*)&reuse,  sizeof (reuse));
		//Modificamos los atributos del socket indicando ip y puerto local
		midireccion6.sin6_family = AF_INET6; //especifica que es de la familia internet
		midireccion6. sin6_port = htons(puerto); //htons y htonl sirve para que no se den vuelta los bytes al momento de compilar. 8080 es el puerto en el que va a escuchar
		inet_pton(AF_INET6, "::1", &midireccion6.sin6_addr); //Con esta función nos evitamos tener que pasar a hexa o binario el IP
		//Bindeamos el socket
		if ( bind(descsocket, (struct sockaddr*) &midireccion6, sizeof(midireccion6) ) < 0 ){
			perror("No se pudo bindear el socket");
			return -1;
		}
		//Lo dejamos escuchando
		if ( listen(descsocket, 10) < 0 ){ //10 son los clientes que pueden estar esperando por un accept
			perror("Error en listen.");
			return -1;
		}
	}else{
		printf("Creando socket...\n");
		//Creamos el SOCKET
		descsocket = socket(PF_INET, SOCK_STREAM, 0); //Creo el socket del tipo PF_INET (Internet), SOCK_STREAM (TCP)
		if (descsocket < 0){
			perror("No se creo el socket");
			return -1;
		}
		//Modificamos las opciones del socket para que podamos iniciarlo nuevamente sin esperar en el caso que se cierre
		setsockopt(descsocket, SOL_SOCKET, SO_REUSEADDR,  (char*)&reuse,  sizeof (reuse));
		//Modificamos los atributos del socket indicando ip y puerto local
		midireccion.sin_family = AF_INET; //especifica que es de la familia internet
		midireccion. sin_port = htons(puerto); //htons y htonl sirve para que no se den vuelta los bytes al momento de compilar. 8080 es el puerto en el que va a escuchar
		inet_pton(AF_INET, ipserver, &midireccion.sin_addr.s_addr); //Con esta función nos evitamos tener que pasar a hexa o binario el IP
		//Bindeamos el socket
		if ( bind(descsocket, (struct sockaddr*) &midireccion, sizeof(midireccion) ) < 0 ){
			perror("No se pudo bindear el socket");
			return -1;
		}
		//Lo dejamos escuchando
		if ( listen(descsocket, 10) < 0 ){ //10 son los clientes que pueden estar esperando por un accept
			perror("Error en listen.");
			return -1;
		}
	}

	printf("Aceptando peticiones...\n");

	//Armo estructura con los argumentos que quiero pasarle al hilo
	argumentos.xbee = xbee;
	argumentos.dirraiz = dirraiz;
	argumentos.sd = -1;

	printf("\nLANZADO HILO PARA ESCUCHAR COLA\n");
	if ( (pthread_create(&idhilo, NULL, escucharCola, (void*)&argumentos)) != 0 ) {
		perror("Hilo No Creado");
	}


	//Dejamos el programa en un bucle infinito aceptando conecciones
	while( (newdescsocket = accept( descsocket,NULL, 0)) > 0 ){ //Mientras no haya problemas al aceptar una conexión

		//Agrego a la estructura el nuevo descriptor de socket
		argumentos.sd = newdescsocket;//Copio el descriptor del socket a la estructura
		ptrarg = malloc(sizeof(arg_struct));//Reservo espacio en memoria para copiar la estructura
		*ptrarg = argumentos;//copio la estructura a otra parte de la memoria

		printf("\nCliente conectado!\n");
		if ( (pthread_create(&idhilo, NULL, atenderconexion, (void*)ptrarg)) != 0 ) {
			perror("Hilo No Creado");
		}

	}

	//Si sale del bucle es porque hubo un problema al aceptar una conexión
	printf("Error al aceptar conexión\n");
	return -1;

}



