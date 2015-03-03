#include <stdio.h>
#include <unistd.h>
#include <string.h>

void escribirensocket(int sd,char *msg){ //Esta funcion simplemente calcula el tamaño de la string que se desea escribir en el socket y la escribe
	int tam = strlen(msg);
	if(write(sd,msg,tam) == -1){
		printf("Error al intentar escribir en socket.\n");
	}
}
