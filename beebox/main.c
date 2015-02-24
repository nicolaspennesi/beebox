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


//-----------------------------------
//-- VARIABLESGLOBALES Y ESTRUCTURAS
//-----------------------------------

//Estructura para mandar argumentos a los hilos nuevos
typedef struct {
	int *ptrsd;//Puntero para hacer malloc. (Descriptor de socket)
	struct xbee *xbee; //Descriptor a xbee conectado al servidor
	char *dirraiz;//Directorio raiz
} arg_struct;

char *dirbeeboxmqsend; //Ruta donde está el ejecutable que escribirá las tareas en la cola (para cron) (se setea en el archivo de configuración)
//------------------
//-- FUNCIONES
//------------------

//Declaro funcion antes del main
void* atenderconexion (void*);
void* escucharCola (void*);

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


	//Setea la conexión con el xbee coordinador por puerto serie
	if ((err = xbee_setup(&xbee, "xbeeZB", puertoserie, baudios)) != XBEE_ENONE) {
		fprintf(stderr, "ERROR AL INTENTAR ABRIR CONEXIÓN POR PUERTO SERIE: %d (%s)\n", err, xbee_errorToStr(err));
		return err;
	}
	printf ("Conexión con coordinador establecida en %s a %d baudios.\n", puertoserie, baudios);


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
	argumentos.ptrsd = NULL;

	printf("\nLANZADO HILO PARA ESCUCHAR COLA\n");
	if ( (pthread_create(&idhilo, NULL, escucharCola, (void*)&argumentos)) != 0 ) {
		perror("Hilo No Creado");
	}


	//Dejamos el programa en un bucle infinito aceptando conecciones
	while( (newdescsocket = accept( descsocket,NULL, 0)) > 0 ){ //Mientras no haya problemas al aceptar una conexión

		//Agrego a la estructura el nuevo descriptor de socket
		//CAMBIAR POR MALLOC
		argumentos.ptrsd = malloc(sizeof(int));
		*argumentos.ptrsd = newdescsocket;

		//argumentos.sd = newdescsocket;

		printf("\nCliente conectado!\n");
		if ( (pthread_create(&idhilo, NULL, atenderconexion, (void*)&argumentos)) != 0 ) {
			perror("Hilo No Creado");
		}

	}

	//Si sale del bucle es porque hubo un problema al aceptar una conexión
	printf("Error al aceptar conexión\n");
	return -1;

}


void *atenderconexion(void* argumentos){

	arg_struct *args = (arg_struct*)argumentos;//Recupero el puntero a la estructura con los argumentos

	int sd = *args->ptrsd;//Descriptor al nuevo socket para que el hilo responda
	char buffer[4096];
	char peticion[4096];//Esta variable se usa para mostrar la peticón completa en el caso de que el cliente vaya escribiendo en el buffer de a partes
	int leido = -1;
	//int dobleenter = 0; //Sirve para chequear si se hizo un dobleenter en el socket

	http_t http = {"NA","NA","NA","NA"}; //Inicializo la estructura vacía.

	pthread_detach(pthread_self());//Desasocia el hilo del hilo principal (si se cuelga el hilo, no se va a colgar el proceso principal)

	printf("HILO ENTRO EN RUTINA\n");
	printf("Socket: %d\n",sd);

	memset(buffer, 0, sizeof buffer); //Llena el buffer con 0
	memset(peticion, 0, sizeof peticion); //Llena la peticion con 0

	//--------------------------------------------------------------
	//----USAR EN CASO DE RECIBIR LA PETICÓN POR PARTES (NETCAT)---- (Tiene el problema de no salir del while ante una consulta post)
	//--------------------------------------------------------------
	// //Hacemos el while en el caso de conectar al server por medio de netcat (que va escribiendo en el buffer linea por linea, no como el navegadorque escribe toda la petición junta).
	// while (dobleenter != 1 && leido != 0){//Hasta que no haya un doble enter "\n\n" o "\r\n\r\n"
	// 	//Leemos del socket la peticion del cliente
	// 	if ( (leido = read(sd, buffer, sizeof(buffer))) < 0){
	// 		perror("Error al leer del socket\n");
	// 		//pthread_exit(NULL);
	// 	}

	//  //PARA DEBUGUEAR
	// 	printf("\nPetición buffer:\n");
	// 	printf("---------------------\n");
	// 	printf("%s\n",buffer);
	// 	printf("---------------------\n");
	// 	printf("Fin Petición del cliente.\n\n");

	// 	printf("Socket: %d\n",sd);
	// 	printf("Leido: %d\n", leido);
	// 	printf("Last1: %d\n", buffer[leido-1]);
	// 	printf("Last2: %d\n", buffer[leido-2]);
	// 	printf("Last3: %d\n", buffer[leido-3]);
	// 	printf("Last4: %d\n", buffer[leido-4]);

	// 	strcat(peticion, buffer);//Concatena el contenido de peticion con el de buffer y lo guarda en petición

	// 	//Si es un \r\n\r\n (dobleenter con retorno de carro) devuelve 0
	// 	if (strlen(peticion) >= 4){ //Para que no lea índices negativos en el buffer
	// 		if ( (peticion[strlen(peticion)-4] == '\r') && (peticion[strlen(peticion)-3] == '\n') && (peticion[strlen(peticion)-2] == '\r') && (peticion[strlen(peticion)-1] == '\n') ){
	// 			dobleenter = 1; //Sale del bucle
	// 		}
	// 	}
	// 	//Si es un \n\n (dobleenter) devuelve 0
	// 	if ((strlen(peticion) >= 2) && (dobleenter != 0)){//Para que no lea índices negativos en el buffer
	// 		if ( (peticion[strlen(peticion)-2] == '\n') && (peticion[strlen(peticion)-1] == '\n') ){
	// 			dobleenter = 1; //Sale del bucle
	// 		}
	// 	}

	// 	memset(buffer, 0, sizeof buffer); //Llena el buffer con 0
	// }

	//-----------------------------------------------------------------
	//----USAR EN CASO DE RECIBIR TODA LA PETICÓN JUNTA (NAVEGADOR)----
	//-----------------------------------------------------------------
	//Leemos del socket la peticion del cliente
	if ( (leido = read(sd, peticion, sizeof(peticion))) < 0){
		perror("Error al leer del socket\n");
	}

	//Vemos la Petición del cliente
	printf("\nPetición del cliente:\n");
	printf("---------------------\n");
	printf("%s\n",peticion);
	printf("---------------------\n");
	printf("Fin Petición del cliente.\n\n");

	//Parseamos la peticion y guardamos en una estructura los datos que solicita el cliente
	http_parse_req(peticion, leido, &http);

	//Llamamos a la funcion que escribe una respuesta en el socket
	if (http_send_resp(sd, &http, args->dirraiz, args->xbee) != 0){
		printf("No se puedo responder al cliente.\n");
	}

	//printf("Socket: %d\n",sd);
	close(sd); //Cerramos el descriptor

	//free(arg);
	pthread_exit(NULL);
}

void* escucharCola (void* argumentos){

	arg_struct *args = (arg_struct*)argumentos;//Recupero el puntero a la estructura con los argumentos

	mqd_t descCola; //mqd_t es un tipo de variable parecido a un int
	struct mq_attr atributosCola; //declaro la estructura para contener los atributos de lacolade mensajes

	json_t *jasonData, *remotoData, *pinData, *comandoData; //Variables donde se guardará el json parseado
	json_error_t error; //Variable donde se guardarán errores del parseadorn en caso de producirse
	const char *remoto_valor, *pin_valor, *comando_valor; //Variables donde se guardarán los valores de lo recibido por jsony
	char remoto[20], pin[5], comando[10]; //Para evitar problema con tipo de variable "const"

	if ((descCola = mq_open("/beeboxmq",O_RDONLY|O_CREAT,0666,NULL)) < 0){ //Abre la cola y si no existe la crea. Si al abrir/crear la cola se produce un error mq_open va a devolver -1.
		fprintf(stderr, "ERROR AL ABRIR COLA DE MENSAJES\n");
		pthread_exit((void*)0);
	}

	pthread_detach(pthread_self());//Desasocia el hilo del hilo principal (si se cuelga el hilo, no se va a colgar el proceso principal)


	mq_getattr (descCola, &atributosCola); //atributos es una estructura donde se van a guardar los atributos de la cola de mensajes

	//cambiar por malloc
	char bufferCola[atributosCola.mq_msgsize]; //declaro el buffer del tamaño máximo posible para un mensaje (normalmente 8k)

	//si no hay mensajes en la cola, el programa se va a quedar esperando a que aparezca un mensaje en la cola
	while(1){ //Bucle infinito leyendo mensajes
		if (mq_receive (descCola, bufferCola, sizeof bufferCola, NULL) < 0 ){ //se lee un mensaje de la cola, el tercer argumento es la prioridad, en este caso es NULL, lee de mayor a menor.
			fprintf(stderr, "ERROR AL INTENTAR LEER DE LA COLA DE MENSAJES\n");
			pthread_exit((void*)0);
		}
		printf("MENSAJE RECIBIDO, PARSENADO JSON...\n");
		jasonData = json_loads(bufferCola, 0, &error);
		//Comprobamos que se haya parseado correctamente
		if(!jasonData){
			fprintf(stderr, "ERROR AL PARSEAR JSON. LÍNEA %d: %s\n", error.line, error.text);
			pthread_exit((void*)0);
		}
		//Obtenemos los valores de la estructura json y los guardamos en las variables correspondientes
		remotoData = json_object_get(jasonData, "remoto");
		remoto_valor = json_string_value(remotoData);
		strcpy(remoto, remoto_valor);
		pinData = json_object_get(jasonData, "pin");
		pin_valor = json_string_value(pinData);
		strcpy(pin, pin_valor);
		comandoData = json_object_get(jasonData, "comando");
		comando_valor = json_string_value(comandoData);
		strcpy(comando, comando_valor);

		printf("Mensaje Entró a la Cola: Remoto: %s, Pin: %s, Comando: %s\n", remoto, pin, comando);
		ejecutarComando(args->xbee, remoto, pin, *comando);
	}

	pthread_exit(NULL);
}



