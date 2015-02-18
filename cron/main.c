//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------
//-- CONSTANTES
//------------------

#define RUTA_EJECUTABLE "/home/nicolas/Escritorio/beebox/colaescribir/beeboxmqsend" //Indica la ruta al archivo que ejecutará la tarea de cron

//------------------
//-- FUNCIONES
//------------------

void escaparAsteriscos(char *texto){
	char textoNuevo[400] = "";
	int i = 0;
	int j = 0;

	//Recorre el texto copiando caracter por caracter a una nueva cadena, cuando hay un asteriscos, en la nueva cadena agrega una \ antes para escaparlo
	while(texto[i] != '\0'){
		if(texto[i] == '*'){
			textoNuevo[j] = '\\';
			j++;
			textoNuevo[j] = texto[i];
		} else {
			textoNuevo[j] = texto[i];
		}
		i++;
		j++;
	}
	//Cerramos el string nuevo
	textoNuevo[j] = '\0';
	//copia el resultado de la nueva cadena a la vieja (con los asteriscos escapados)
	strcpy(texto, textoNuevo);
}

void agregarTarea(){
	int status;

	char minuto[10] = "*"; //(0 - 59)
	char hora[10] = "*"; //(0 - 23)
	char diaDelMes[10] = "*";//(1 - 31)
	char mes[10] = "*"; //(1 - 12)
	char diaDeLaSemana[10] = "*"; //(0 - 6) (Domingo=0)

	char *remoteaddr = "0013A2004086A3FF"; //Setea la dirección por del nodo remoto
	char *comando = "c"; //El comando por defecto es cambiar el estado del pin
	char *pin = "1"; //El pin por defecto es el pin 1

	char comandoSistema[500] = ""; //Comando a ejecutar por el sistema, 500 caracteres como máximo
	char entradaCron[400] = ""; //Tarea que quedará escrita en el archivo de cron
	char entradaCronEscapada[400] = ""; //Tarea de cron con caracteres escapados para poder utilizar el comanod grep -v

	strcpy(minuto, "*/1");

	strcat(entradaCron, minuto);
	strcat(entradaCron, " ");
	strcat(entradaCron, hora);
	strcat(entradaCron, " ");
	strcat(entradaCron, diaDelMes);
	strcat(entradaCron, " ");
	strcat(entradaCron, mes);
	strcat(entradaCron, " ");
	strcat(entradaCron, diaDeLaSemana);
	strcat(entradaCron, " ");
	strcat(entradaCron, RUTA_EJECUTABLE);
	strcat(entradaCron, " -r ");
	strcat(entradaCron, remoteaddr);
	strcat(entradaCron, " -p ");
	strcat(entradaCron, pin);
	strcat(entradaCron, " -c ");
	strcat(entradaCron, comando);

	strcpy(entradaCronEscapada, entradaCron);
	escaparAsteriscos(entradaCronEscapada);

	strcat(comandoSistema, "(crontab -l | grep -v \""); //coneste grep evitamos que el comando se repita si ya está cargado en el archivo de cron
	strcat(comandoSistema, entradaCronEscapada);
	strcat(comandoSistema, "\" && echo \"");
	strcat(comandoSistema, entradaCron);
	strcat(comandoSistema, "\") | crontab -");

	status = system(comandoSistema);
	if(status != 0){
		fprintf(stderr, "ERROR AL INTENTAR AGREGAR TAREA A CRON (%d)\n", status);
	}
}

void quitarTarea(){
	int status;

	char minuto[10] = "*"; //(0 - 59)
	char hora[10] = "*"; //(0 - 23)
	char diaDelMes[10] = "*";//(1 - 31)
	char mes[10] = "*"; //(1 - 12)
	char diaDeLaSemana[10] = "*"; //(0 - 6) (Domingo=0)
	char comandoSistema[500] = ""; //Comando a ejecutar por el sistema, 500 caracteres como máximo
	char entradaCronEscapada[400] = ""; //Tarea de cron con caracteres escapados para poder utilizar el comanod grep -v

	char *remoteaddr = "0013A2004086A3FF"; //Setea la dirección por del nodo remoto
	char *comando = "c"; //El comando por defecto es cambiar el estado del pin
	char *pin = "1"; //El pin por defecto es el pin 1

	strcpy(minuto, "*/1");

	strcat(entradaCronEscapada, minuto);
	strcat(entradaCronEscapada, " ");
	strcat(entradaCronEscapada, hora);
	strcat(entradaCronEscapada, " ");
	strcat(entradaCronEscapada, diaDelMes);
	strcat(entradaCronEscapada, " ");
	strcat(entradaCronEscapada, mes);
	strcat(entradaCronEscapada, " ");
	strcat(entradaCronEscapada, diaDeLaSemana);
	strcat(entradaCronEscapada, " ");
	strcat(entradaCronEscapada, RUTA_EJECUTABLE);
	strcat(entradaCronEscapada, " -r ");
	strcat(entradaCronEscapada, remoteaddr);
	strcat(entradaCronEscapada, " -p ");
	strcat(entradaCronEscapada, pin);
	strcat(entradaCronEscapada, " -c ");
	strcat(entradaCronEscapada, comando);

	escaparAsteriscos(entradaCronEscapada);

	//TODO Para que funcione bien el grep -V hay que escapar los asteriscos con \*
	strcat(comandoSistema, "(crontab -l | grep -v \"");
	strcat(comandoSistema, entradaCronEscapada);
	strcat(comandoSistema, "\") | crontab -");

	status = system(comandoSistema);
	if(status != 0){
		fprintf(stderr, "ERROR AL INTENTAR QUITAR TAREA DE CRON (%d)\n", status);
	}
}


int main(int argc, char *const argv[]){

	agregarTarea();
	//quitarTarea();
	printf("FIN\n");
	return 0;
}
