//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//------------------
//-- CONSTANTES
//------------------

#define RUTA_EJECUTABLE "/home/nicolas/Escritorio/beebox/xbee/xbee" //Indica la ruta al archivo que ejecutará la tarea de cron

//------------------
//-- FUNCIONES
//------------------

void escaparAsteriscos(char *texto){
	char textoNuevo[10] = "";
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
	char comandoSistema[500] = ""; //Comando a ejecutar por el sistema, 500 caracteres como máximo

	char *remoteaddr = "0013A2004086A3FF"; //Setea la dirección por del nodo remoto
	char *comando = "c"; //El comando por defecto es cambiar el estado del pin
	char *pin = "1"; //El pin por defecto es el pin 1

	strcpy(minuto, "*/1");

	strcat(comandoSistema, "(crontab -l && echo \"");
	strcat(comandoSistema, minuto);
	strcat(comandoSistema, " ");
	strcat(comandoSistema, hora);
	strcat(comandoSistema, " ");
	strcat(comandoSistema, diaDelMes);
	strcat(comandoSistema, " ");
	strcat(comandoSistema, mes);
	strcat(comandoSistema, " ");
	strcat(comandoSistema, diaDeLaSemana);
	strcat(comandoSistema, " ");
	strcat(comandoSistema, RUTA_EJECUTABLE);
	strcat(comandoSistema, " -r ");
	strcat(comandoSistema, remoteaddr);
	strcat(comandoSistema, " -p ");
	strcat(comandoSistema, pin);
	strcat(comandoSistema, " -c ");
	strcat(comandoSistema, comando);
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

	char *remoteaddr = "0013A2004086A3FF"; //Setea la dirección por del nodo remoto
	char *comando = "c"; //El comando por defecto es cambiar el estado del pin
	char *pin = "1"; //El pin por defecto es el pin 1

	strcpy(minuto, "*/1");

	//TODO Para que funcione bien el grep -V hay que escapar los asteriscos con \*
	strcat(comandoSistema, "(crontab -l | grep -v \"");
	escaparAsteriscos(minuto);
	strcat(comandoSistema, minuto);
	strcat(comandoSistema, " ");
	escaparAsteriscos(hora);
	strcat(comandoSistema, hora);
	strcat(comandoSistema, " ");
	escaparAsteriscos(diaDelMes);
	strcat(comandoSistema, diaDelMes);
	strcat(comandoSistema, " ");
	escaparAsteriscos(mes);
	strcat(comandoSistema, mes);
	strcat(comandoSistema, " ");
	escaparAsteriscos(diaDeLaSemana);
	strcat(comandoSistema, diaDeLaSemana);
	strcat(comandoSistema, " ");
	strcat(comandoSistema, RUTA_EJECUTABLE);
	strcat(comandoSistema, " -r ");
	strcat(comandoSistema, remoteaddr);
	strcat(comandoSistema, " -p ");
	strcat(comandoSistema, pin);
	strcat(comandoSistema, " -c ");
	strcat(comandoSistema, comando);
	strcat(comandoSistema, "\") | crontab -");

	status = system(comandoSistema);
	if(status != 0){
		fprintf(stderr, "ERROR AL INTENTAR QUITAR TAREA DE CRON (%d)\n", status);
	}
}


int main(int argc, char *const argv[]){

	agregarTarea();
	quitarTarea();
	printf("FIN\n");
	return 0;
}
