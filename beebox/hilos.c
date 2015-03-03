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

//------------------
//-- FUNCIONES
//------------------

void *atenderconexion(void* argumentos){

	arg_struct *args = (arg_struct*)argumentos;//Recupero el puntero a la estructura con los argumentos

	int sd = args->sd;//Descriptor al nuevo socket para que el hilo responda
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

	free(argumentos);
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