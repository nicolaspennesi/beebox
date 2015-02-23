//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

int agregarTareaCron(char *rutaEjecutable, char *minuto, char *hora, char *diaDelMes, char *mes, char *diaDeLaSemana, char *remoteaddr, char *comando, char *pin){
	pid_t child_pid;
	int status;

	char comandoSistema[500] = ""; //Comando a ejecutar por el sistema, 500 caracteres como máximo
	char entradaCron[400] = ""; //Tarea que quedará escrita en el archivo de cron
	char entradaCronEscapada[400] = ""; //Tarea de cron con caracteres escapados para poder utilizar el comanod grep -v

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
	strcat(entradaCron, rutaEjecutable);
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
	strcat(comandoSistema, "\" ; echo \""); //el ";" en lugar del "&&" es para que el comando echo se ejecute incluso si el anterior falla (por ejemplo si no existe el archivo de crontab)
	strcat(comandoSistema, entradaCron);
	strcat(comandoSistema, "\") | crontab -");

	//Hacemos un fork del proceso para poder hacer el exec sin que pise la ejecución principal
	if((child_pid = fork()) < 0 ){
		perror("ERROR AL INTENTAR HACER FROK\n");
		return -1;
	}
	//Si es el proceso hijo, hace el exec
	if(child_pid == 0){

		status = execlp("bash", "bash", "-c", comandoSistema, (char *)0);
		fprintf(stderr, "ERROR AL INTENTAR AGREGAR TAREA A CRON (%d).\n", status);//Si se ejecuta esta línea quiere decir que el exec falló
		return -1;
	}else{
		wait(&status);
		if (status != 0){
			fprintf(stderr, "ERROR AL INTENTAR AGREGAR TAREA A CRON (%d).\n", status);//Si se ejecuta esta línea quiere decir que el exec falló
			return -1;
		}
		printf("TAREA PROGRAMADA\n");
		return 0;
	}

}

int quitarTareaCron(char *rutaEjecutable, char *minuto, char *hora, char *diaDelMes, char *mes, char *diaDeLaSemana, char *remoteaddr, char *comando, char *pin){
	pid_t child_pid;
	int status;

	char comandoSistema[500] = ""; //Comando a ejecutar por el sistema, 500 caracteres como máximo
	char entradaCronEscapada[400] = ""; //Tarea de cron con caracteres escapados para poder utilizar el comanod grep -v

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
	strcat(entradaCronEscapada, rutaEjecutable);
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

	//Hacemos un fork del proceso para poder hacer el exec sin que pise la ejecución principal
	if((child_pid = fork()) < 0 ){
		perror("ERROR AL INTENTAR HACER FROK\n");
		return -1;
	}
	//Si es el proceso hijo, hace el exec
	if(child_pid == 0){

		status = execlp("bash", "bash", "-c", comandoSistema, (char *)0);
		fprintf(stderr, "ERROR AL INTENTAR AGREGAR TAREA A CRON (%d).\n", status);//Si se ejecuta esta línea quiere decir que el exec falló
		return -1;
	}else{
		wait(&status);
		if (status != 0){
			fprintf(stderr, "ERROR AL INTENTAR AGREGAR TAREA A CRON (%d).\n", status);//Si se ejecuta esta línea quiere decir que el exec falló
			return -1;
		}
		printf("TAREA ELIMINADA CORRECTAMENTE\n");
		return 0;
	}

}

void listarTareasCron(){
	FILE *fileDesc;
	char linea[1024];
	char tareas[4096] = "";

	//Ejecutamos crontab -l para que nos devuelva la lista de comandos, esta lista se guarda en un "archivo" en memoria apuntado por fileDesc
	fileDesc = popen("crontab -l", "r"); //La "r" es de solo lectura
	if (fileDesc == NULL) {
		fprintf(stderr, "ERROR NO SE PUDO OBTENER LA LISTA DE COMANDOS\n");
		return;
	}

	//Leemos línea por linea el archivo en memoria y vamos guardándolas en la variable tareas hasta llegar al final del archivo
	while (fgets(linea, sizeof(linea), fileDesc) != NULL) {
		strcat(tareas, linea);
	}

	//Mostramos todas las tareas
	printf("%s", tareas);

	//Cerramos el "archivo"
	pclose(fileDesc);
}

int main(int argc, char *const argv[]){

	char minuto[10] = "*"; //(0 - 59)
	char hora[10] = "*"; //(0 - 23)
	char diaDelMes[10] = "*";//(1 - 31)
	char mes[10] = "*"; //(1 - 12)
	char diaDeLaSemana[10] = "*"; //(0 - 6) (Domingo=0)

	char *remoteaddr = "0013A2004086A3FF"; //Setea la dirección por del nodo remoto
	char *comando = "c"; //El comando por defecto es cambiar el estado del pin
	char *pin = "1"; //El pin por defecto es el pin 1

	strcpy(minuto, "*/1"); //Seteo que se ejecute cada un minuto

	agregarTareaCron(RUTA_EJECUTABLE, minuto, hora, diaDelMes, mes, diaDeLaSemana, remoteaddr, comando, pin);
	//quitarTareaCron(RUTA_EJECUTABLE, minuto, hora, diaDelMes, mes, diaDeLaSemana, remoteaddr, comando, pin);
	//listarTareasCron();

	printf("FIN\n");
	return 0;
}
