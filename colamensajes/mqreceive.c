//compilar con gcc mqreceive.c -o recibir -lrt
#include <stdio.h>
#include <string.h>
#include <mqueue.h>
#include <errno.h>

int main (int argc, char* const argv[]){

	mqd_t descCola; //mqd_t es un tipo de variable parecido a un int
	struct mq_attr atributosCola; //declaro la estructura para contener los atributos de lacolade mensajes

	if ((descCola = mq_open("/mensajes",O_RDONLY|O_CREAT,0666,NULL)) < 0){ //si al crear la cola se produce un error (mq_open va a devolver -1)
		fprintf(stderr, "ERROR AL ABRIR COLA DE MENSAJES\n");
		return -1;
	}

	mq_getattr (descCola, &atributosCola); //atributos es una estructura donde se van a guardar los atributos de la cola de mensajes

	char bufferCola[atributosCola.mq_msgsize]; //declaro el buffer del tamaño máximo posible para un mensaje (normalmente 8k)

	//si no hay mensajes en la cola, el programa se va a quedar esperando a que aparezca un mensaje en la cola
	while(1){ //Bucle infinito leyendo mensajes
		if (mq_receive (descCola, bufferCola, sizeof bufferCola, NULL) < 0 ){ //se lee un mensaje de la cola, el tercer argumento es la prioridad, en este caso es NULL, lee de mayor a menor.
			fprintf(stderr, "ERROR AL INTENTAR LEER DE LA COLA DE MENSAJES\n");
			return -1;
		}
		printf ("Mensaje leido: %s\n",bufferCola);
	}

	return 0;
}
