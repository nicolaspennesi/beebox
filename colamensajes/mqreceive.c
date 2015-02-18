//compilar con gcc mqreceive.c -o recibir -lrt -ljansson
#include <stdio.h>
#include <string.h> //Para el manejo de cadenas
#include <mqueue.h> //Para la cola de mensajes
#include <errno.h> //Para mostrar errores
#include <jansson.h> //Para parsear json

int main (int argc, char* const argv[]){

	mqd_t descCola; //mqd_t es un tipo de variable parecido a un int
	struct mq_attr atributosCola; //declaro la estructura para contener los atributos de lacolade mensajes

	json_t *jasonData, *remoto, *pin, *comando; //Variables donde se guardará el json parseado
	json_error_t error; //Variable donde se guardarán errores del parseadorn en caso de producirse
	const char *remoto_valor, *pin_valor, *comando_valor; //Variables donde se guardarán los valores de lo recibido por json

	if ((descCola = mq_open("/beeboxmq",O_RDONLY|O_CREAT,0666,NULL)) < 0){ //Abre la cola y si no existe la crea. Si al abrir/crear la cola se produce un error mq_open va a devolver -1.
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
		printf("MENSAJE RECIBIDO, PARSENADO JSON...\n");
		jasonData = json_loads(bufferCola, 0, &error);
		//Comprobamos que se haya parseado correctamente
		if(!jasonData){
			fprintf(stderr, "ERROR AL PARSEAR JSON. LÍNEA %d: %s\n", error.line, error.text);
			return -1;
		}
		//Obtenemos los valores de la estructura json y los guardamos en las variables correspondientes
		remoto = json_object_get(jasonData, "remoto");
		remoto_valor = json_string_value(remoto);
		pin = json_object_get(jasonData, "pin");
		pin_valor = json_string_value(pin);
		comando = json_object_get(jasonData, "comando");
		comando_valor = json_string_value(comando);

		printf("Mensaje: Remoto: %s, Pin: %s, Comando: %s\n", remoto_valor, pin_valor, comando_valor);
	}

	return 0;
}
