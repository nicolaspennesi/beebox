//compilar con gcc mqsend.c -o enviar -lrt
#include <stdio.h>
#include <string.h> //Para el manejo de cadenas
#include <mqueue.h> //Para la cola de mensajes
#include <errno.h> //Para mostrar errores

int main (int argc, char* const argv[]){

	mqd_t descCola;
	char *mensaje;

	if ((descCola = mq_open("/beeboxmq",O_WRONLY|O_CREAT,0666,NULL)) < 0){ //Abre la cola y si no existe la crea. Si al abrir/crear la cola se produce un error mq_open va a devolver -1.
		fprintf(stderr, "ERROR AL ABRIR COLA DE MENSAJES\n");
		return -1;
	}

	mensaje = "{\"remoto\": \"0013A2004086A3FF\",\"pin\": \"1\",\"comando\": \"c\"}";

	if (mq_send(descCola, mensaje, strlen(mensaje),0) < 0 ){ //se pone un mensaje en la cola de mensajes, strlen(mensaje) es la cantidad de bytes que va a escribir y 0 es la prioridad (si hay varios mensajes en la cola se leen los de prioridad mas alta primero)
		fprintf(stderr, "ERROR AL INTENTAR ESCRIBIR EN LA COLA DE MENSAJES: %s\n", strerror(errno));
		return -1;
	}
	printf("MENSAJE ESCRITO EN LA COLA DE MENSAJES\n");

	mensaje = "{\"remoto\": \"0013A2004086A3FF\",\"pin\": \"2\",\"comando\": \"e\"}";

	if (mq_send(descCola, mensaje, strlen(mensaje),0) < 0 ){ //se pone un mensaje en la cola de mensajes, strlen(mensaje) es la cantidad de bytes que va a escribir y 0 es la prioridad (si hay varios mensajes en la cola se leen los de prioridad mas alta primero)
		fprintf(stderr, "ERROR AL INTENTAR ESCRIBIR EN LA COLA DE MENSAJES: %s\n", strerror(errno));
		return -1;
	}
	printf("MENSAJE ESCRITO EN LA COLA DE MENSAJES\n");

	mensaje = "{\"remoto\": \"0013A2004086A3FF\",\"pin\": \"4\",\"comando\": \"a\"}";

	if (mq_send(descCola, mensaje, strlen(mensaje),0) < 0 ){ //se pone un mensaje en la cola de mensajes, strlen(mensaje) es la cantidad de bytes que va a escribir y 0 es la prioridad (si hay varios mensajes en la cola se leen los de prioridad mas alta primero)
		fprintf(stderr, "ERROR AL INTENTAR ESCRIBIR EN LA COLA DE MENSAJES: %s\n", strerror(errno));
		return -1;
	}
	printf("MENSAJE ESCRITO EN LA COLA DE MENSAJES\n");

	return 0;
}
