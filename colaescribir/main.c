//compilar con gcc mqsend.c -o enviar -lrt
//Probar rápido con:
//./beeboxmqsend -r 0013A2004086A3FF -p 1 -c c
//./enviar -r 0013A2004086A3FF -p 1 -c c
//./enviar -r 0013A2004086A3FF -p 4 -c a

//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <string.h> //Para el manejo de cadenas
#include <mqueue.h> //Para la cola de mensajes
#include <errno.h> //Para mostrar errores
#include <unistd.h> //Para opciones (argumentos) y otras cosas

//------------------
//-- FUNCIONES
//------------------

//Este módulo es llamado por cron y se encarga de meter en la cola de mensajes las acciones que debe realizar el programa principal (codificadas en json).
int main(int argc, char* const argv[]){

	int opcion; //Para el manejo de argumentos

	char *remoteaddr; //Dirección del nodo remoto
	char *pin; //El pin del nodo remoto
	char *comando; //El comando a ejecutar (cambiar, encender o apagar)

	mqd_t descCola; //Descriptor de la cola de mensajes
	struct mq_attr atributosCola; //declaro la estructura para contener los atributos de lacolade mensajes

	if(argc != 7){
		fprintf(stderr, "Uso: %s -r SERIAL_NODO_REMOTO -p NRO_PIN -c COMANDO\n", argv[0]);
		fprintf(stderr, "Comandos posibles: c=cambiar, e=encender, a=apagar.\n\n");
		return 2;
	}

	//Se analizan los argumentos
	while ((opcion = getopt (argc, argv, "r:p:c:")) != -1){
		switch (opcion){
			case 'r': //serial del nodo remoto
				remoteaddr = optarg;
				break;
			case 'p': //pin del nodo remoto
				pin = optarg;
				break;
			case 'c': //comando nodo remoto (c: cambiar, e: encender, a= apagar)
				//Para esta opcion solo se evalúa el primer caracter del argumento
				if(*optarg == 'c' || *optarg == 'e' || *optarg == 'a'){
					comando = optarg; //Guarda el primer caracter del argumento en la variable comando
				} else {
					fprintf (stderr, "El argumento de la opción 'c' sólo puede ser c: cambiar, e: encender, a: apagar.\n");
					return 1;
				}
				break;
			case '?':
				if (optopt == 'f') {
					fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
				} else if (optopt == 'r') {
					fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
				} else if (optopt == 'p') {
					fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
				} else if (optopt == 'c') {
					fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
				} else if (isprint (optopt)) {
					fprintf (stderr, "Opción '-%c' desconocida.\n", optopt);
				}
				return 1;
			default:
				printf ("Argumentos Incorrectos");
				return 1;
		}
	}

	if ((descCola = mq_open("/beeboxmq",O_WRONLY|O_CREAT,0666,NULL)) < 0){ //Abre la cola y si no existe la crea. Si al abrir/crear la cola se produce un error mq_open va a devolver -1.
		fprintf(stderr, "ERROR AL ABRIR COLA DE MENSAJES\n");
		return -1;
	}
	mq_getattr (descCola, &atributosCola); //atributos es una estructura donde se van a guardar los atributos de la cola de mensajes
	char mensaje[atributosCola.mq_msgsize]; //declaro el la variable para el mensaje con el tamaño máximo posible que admite la cola (normalmente 8k)
	//printf("TAMAÑO MAX MENSAJES: %d\n", atributosCola.mq_msgsize); //Devuelve 8192

	//Armamos json para colocar en la cola
	strcpy(mensaje, "");
	strcat(mensaje, "{\"remoto\": \"");
	strcat(mensaje, remoteaddr);
	strcat(mensaje, "\",\"pin\": \"");
	strcat(mensaje, pin);
	strcat(mensaje, "\",\"comando\": \"");
	strcat(mensaje, comando);
	strcat(mensaje, "\"}");

	//Enviamos el mensaje
	if (mq_send(descCola, mensaje, strlen(mensaje),0) < 0 ){ //se pone un mensaje en la cola de mensajes, strlen(mensaje) es la cantidad de bytes que va a escribir y 0 es la prioridad (si hay varios mensajes en la cola se leen los de prioridad mas alta primero)
		fprintf(stderr, "ERROR AL INTENTAR ESCRIBIR EN LA COLA DE MENSAJES: %s\n", strerror(errno));
		return -1;
	}
	printf("MENSAJE ESCRITO EN LA COLA DE MENSAJES: Remoto: %s, Pin: %s, Comando: %s\n", remoteaddr, pin, comando);

	return 0;
}
