//Server desde 0
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>//Para el open
#include <stdlib.h>
#include <sys/types.h> //Para Socket
#include <sys/socket.h> //Para Socket
#include <arpa/inet.h> //Para Socket
#include <errno.h> //Para perror
#include <pthread.h> //Para Hilos
#include <string.h> //Para memset

#include "http.h"

//Variables Globales
char* dirraiz = "/home/nicolas/www";
int puerto = 8080;
//pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //Creo e inicializo una variable mutex en una sola linea

//Declaro funcion antes del main
void* atenderconexion (void*);

int main(int argc, char *const argv[]){

	int descsocket; //Descriptor del Socket (descriptor pasivo, tiene solo dos valores [IP y PUERTO local])
	int newdescsocket; //Descriptor para las conecciones (descriptor activo, tiene los 4 valores [IP LOCAL Y de CLIENTE, PUERTO LOCAL Y de CLIENTE])
	struct sockaddr_in midireccion = {};//Estructura en donde estan los datos del socket (como ip y puerto)
	struct sockaddr_in6 midireccion6 = {};//Estructura en donde estan los datos del socket (como ip y puerto)
	int reuse = 1;//Para setsockopt. Indica el tiempo en el que se podrá volver a iniciar el servidor en caso de cerrarse
	pthread_t idhilo; //Esta variable contendrá el id del hilo creado
	int opcion; //Para el manejo de argumentos
	int ipv6 = 0;
	int leido;
	char buffer[1024];
	char* archconfig = "server.cfg";//Puntero al nombre del archivo de configuración. Por defecto es server.cfg
	int acd; //Descriptor del archivo de configuracion
	char* puertostring;//Para luego convertirlo a int
	int* ptr;//Puntero para malloc
	//int i = 0;


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
	//Parseamos para dejar en la variable puerto el puerto especificado en el archivo de configuracion (pero en string)
	puertostring = strtok (buffer, "=");
	puertostring = strtok (NULL, " ");
	puerto = atoi(puertostring);
	printf ("El puerto es: %d\n", puerto);
	//Parseamos para dejar en la variable dirraiz el directorio raiz especificado en el archivo de configuracion (pero en string)
	dirraiz = strtok (NULL, "=");
	dirraiz = strtok (NULL, "\n");
	printf ("El directorio raiz es: %s\n", dirraiz);


	if (ipv6 == 1){

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
		inet_pton(AF_INET, "127.0.0.1", &midireccion.sin_addr.s_addr); //Con esta función nos evitamos tener que pasar a hexa o binario el IP

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

	printf("Aceptando conexiones...\n\n");


	//Dejamos el programa en un bucle infinito aceptando conecciones
	while( (newdescsocket = accept( descsocket,NULL, 0)) > 0 ){ //Mientras no haya problemas al aceptar una conexión

		//pthread_mutex_lock (&mutex); //Bloquea el mutex

		ptr = malloc(sizeof(int));
		*ptr = newdescsocket;

		printf("\nCliente conectado.\n");
		if ( (pthread_create(&idhilo, NULL, atenderconexion, (void*)ptr)) != 0 ) {
			perror("Hilo No Creado");
		}

	}

	//Si sale del bucle es porque hubo un problema al aceptar una conexión
	printf("Error al aceptar conexión\n");
	return -1;


}


void* atenderconexion(void* arg){

	int sd = *(int *)arg;
	char buffer[4096];
	int leido = 0;
	int dobleenter = 1; //Sirve para chequear si se hizo un dobleenter en el socket
	http_t http ={"NA","NA","NA"}; //Inicializo la estructura vacía.
	int ret;

	pthread_detach(pthread_self());//Desacocia el hilo del hilo principal

	printf("HILO ENTRO EN RUTINA\n");
	printf("Socket: %d\n",sd);

	memset(buffer, 0, sizeof buffer); //Llena el buffer con 0

	//pthread_mutex_unlock (&mutex); //Desbloquea el mutex

	while (dobleenter != 0){//Hasta que no haya un doble enter "\n\n"
		//Leemos del socket la peticion del cliente
		if ( (leido = read(sd, buffer, sizeof(buffer))) < 0){
			perror("Error al leer del socket");
			//pthread_exit(NULL);
		}


		//Vemos la consulta del cliente
		printf("Consulta del cliente:\n");
		printf("%s\n",buffer);


		//Parseamos y guardamos en una estructura los datos que solicita el cliente
		dobleenter = http_parse_req(buffer, leido, &http, dobleenter);
		//printf("Valor Devuelto: %d\n",dobleenter);
		memset(buffer, 0, sizeof buffer); //Llena el buffer con 0
	}

	//Llamamos a la funcion que escribe una respuesta en el socket
	ret = http_send_resp(sd, &http, dirraiz);
	if (ret != 0){
		printf("La respuesta no fue como se esperaba.\n");
	}

	//printf("Socket: %d\n",sd);
	close(sd); //Cerramos el descriptor

	free(arg);
	pthread_exit(NULL);
}



