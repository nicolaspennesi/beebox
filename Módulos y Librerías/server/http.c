#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h> //Para todos los str
#include <sys/types.h> //Para open
#include <sys/stat.h> //Para open
#include <fcntl.h> //Para open

#include "http.h"
#include "mime.h"

int http_parse_req(char *buffer, int leido, http_t* http, int dobleenter) 
{
	char* linea = NULL; //Contendrá la primera linea de lo que escriba el cliente en el socket
	char* parse;
	char* ptr1;
	char* ptr2;
	char* ptr3;
	char archivo[256];
	char bufferoriginal[4096];//Para copiar el contenido del buffer y mantener el original. Tiene el mismo tamaño que el bufer original
	

	strcpy(bufferoriginal, buffer);//Copia el contenido del buffer a bufferoriginal para analizarlo luego ya que strtok destruye el contenido original del buffer
	
	//En el caso de conectar al server por medio de netcat (que va escribiendo en el buffer linea por linea, no como el navegadorque escribe toda la petición junta).
	//Si la linea anterior termino con \n
	if ( (dobleenter == 2) && (bufferoriginal[0] == '\n') && (bufferoriginal[leido-1] == '\n') ){ //Si el read anterior termino en un \n el primer caracter, es el ultimo y es \n
		return 0;			
	}
	
	linea = strtok_r (buffer, "\n", &ptr1);//esto reemplazará el primer \n que encuentre por un \0 así trabajaremos unicamente con la primera linea
	if(linea == NULL){
		printf("No se encontro un salto de linea en el buffer\n");
		return 0;
	}

	//comprobamos que sea una petición HTTP
	parse = strstr(linea," HTTP/");//Busca en la primera linea si existe " HTTP/" y devuelve un puntero a la posicion donde comienza
	if(parse == NULL){
		printf("La petición no es HTTP.\n");
	}else{
		*parse='\0'; //Colocamos un '\0' para en la posición que señala parse. (este '\0' queda inmediatamente despues de la pagina solicitada ej: index.html\0
		parse=NULL; //Volvemos el puntero a NULL
		parse = strtok_r (linea, " ", &ptr2);//Parsea el método
		strcpy(http->metodo,parse);//Lo guarda en la estructura
		parse = strtok_r (NULL, " ", &ptr2);//Parsea el archivo
		//Si no hay nombre de pagina, indicamos por defecto que es index.html
		if( strlen(parse) == 1){ 
			strcat(parse,"index.html");
		}
		strcpy(http->file,parse);//Lo guarda en la estructura
		strcpy(archivo,http->file);//Copia el archivo a una cadena de caracteres nueva así strtok no destruye el de la estructura
		parse = strtok_r (archivo, ".", &ptr3);//Se parsea la extension
		parse = strtok_r (NULL, " ", &ptr3);
		if (parse == NULL){//Si el archivo no tiene extension
			parse = "noext";
		}
		http->mime = mime_gettype(parse);//Se analiza la extensión y se guarda en la estructura el mime type
	}

	//Se analiza si los ultimos 4 caracteres son "\r\n\r\n" (dobleenter)
	//Si es un \r\n\r\n devuelve 0
	if ( (bufferoriginal[leido-4] == '\r') && (bufferoriginal[leido-3] == '\n') && (bufferoriginal[leido-2] == '\r') && (bufferoriginal[leido-1] == '\n') ){
		return 0;
	}
	//Se analiza si los ultimos 2 son "\n"

	if ( bufferoriginal[leido-1] == '\n' ){ //netcat al apretar enter manda al final un 0
		return 2; //Si es asi se devuelve un 2
	}

	return 0;
}


int http_send_resp(int sd, http_t *http, char* dirraiz) {

	printf("Metodo: %s\n",http->metodo);
	printf("File: %s\n",http->file);
	printf("Mime/Type: %s\n",http->mime);


	char dirarchivo[1024];
	char respuesta[1024];
	int tamarch, leido, fd;

	if ( (strcmp(http->metodo, "GET")) && (strcmp(http->metodo, "POST")) ){ //Si el metodo no es ni get ni post
		printf("Error 501 Metodo HTTP no implementado.\n");
		escribirensocket(sd,"HTTP/1.0 501 Not Implemented\r\n");
		escribirensocket(sd,"Content-Type: text/html\r\n\r\n");
		escribirensocket(sd,"<html><head><title>Error 501</title></head>");
		escribirensocket(sd,"<body><h1>Error 501</h1><p>Metodo HTTP no implementado en el servidor.</p><h3>Compu II Server</h3></body></html>");
		return 0;
	}

	strcpy(dirarchivo,dirraiz);
	strcat(dirarchivo,http->file);
	printf("Pedido: %s\n",dirarchivo);
	//Ahora lo abrimos
	fd = open(dirarchivo,O_RDONLY,0);
	//Si no existe el archivo, escribe en el socket un error 404
	if(fd == -1){
		printf("Error 404 Archivo no encontrado.\n");
		escribirensocket(sd,"HTTP/1.0 404 Not Found\r\n");
		escribirensocket(sd,"Content-Type: text/html\r\n\r\n");
		escribirensocket(sd,"<html><head><title>Error 404</head></title>");
		escribirensocket(sd,"<body><h1>Error 404</h1><p>Pagina no encontrada :(</p><h3>Compu II Server</h3></body></html>");
		return 0;
	}else{ //Si el archivo existe
		printf("200 OK! respondiendo peticion para un archivo %s\n", http->file);
		//Obtenemos el tamaño del archivo solicitado
		if( (tamarch = get_tamanioarchivo(fd)) == -1 ){
			printf("Error al obtener el tamaño del archivo.\n");
			return 1;
		}
		memset(respuesta, 0, sizeof respuesta); //Llena el string respuesta con 0
		snprintf(respuesta, sizeof respuesta, "HTTP/1.0 200\r\nContent-Type: %s\r\nContent-Length: %d\r\n\r\n", http->mime, tamarch);
		escribirensocket(sd, respuesta);//Escribe en el socket la respuesta HTTP
		//Comienza a escribir el contenido del archivo en el socket
		while ( (leido = read(fd, respuesta, sizeof respuesta)) > 0) {
			if (leido == -1){
				perror("Error al leer el archivo solicitado");
			}
			if ( (write(sd, respuesta, leido)) == -1){
				printf("Error al intentar escribir en socket %d\n", sd);
			}
		}
		
	}
	close(fd);

	return 0;
}


