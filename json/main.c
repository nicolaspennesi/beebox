//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h> //Para parsear json

//------------------
//-- CONSTANTES
//------------------

#define TEXTO_JSON "{\"remoto\": \"0013A2004086A3FF\",\"pin\": \"1\",\"comando\": \"c\"}"

//------------------
//-- FUNCIONES
//------------------

int main(int argc, char *const argv[]){

	char *jsonText; //json a parsear

	json_t *jasonData, *remoto, *pin, *comando; //Variables donde se guardarán los objetos json parseados
	json_error_t error; //Variable donde se guardarán errores del parseador en caso de producirse
	const char *remoto_valor, *pin_valor, *comando_valor; //Variables donde se guardarán los valores de lo recibido por json

	jsonText = TEXTO_JSON;

	jasonData = json_loads(jsonText, 0, &error);

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

	printf("Remoto: %s, Pin: %s, Comando: %s\n", remoto_valor, pin_valor, comando_valor);

	return 0;
}
