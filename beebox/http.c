#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h> //Para todos los str
#include <sys/types.h> //Para open
#include <sys/stat.h> //Para open
#include <fcntl.h> //Para open
#include <jansson.h> //Para parsear json
#include <xbee.h> // Librería para manejo interno de Xbees

#include "http.h" //Funciones propias
#include "mime.h" //Funciones propias para definir mimetype
#include "utilidades.h" //Funciones propias
#include "xbee.h" //Funciones propias

//Devuelve la extensión del nombre de un archivo
char *get_filename_ext(char *filename) {
	char *dot;

	dot = strrchr(filename, '.'); //Devuelve un puntero al último caracter especificado encontrado dentro de la cadena
	if(!dot || dot == filename) return "noext"; //Si no encontró ninguno devuelve una cadena vacía
	return dot + 1; //Si encontró devuelve un puntero al caracter siguiente (que sería la extensión)
}

//Pasea la petición HTTP y completa la estructura http_t con lo parseado
int http_parse_req(char *buffer, int leido, http_t* http)
{
	char *linea = NULL; //Contendrá la primera linea de lo que escriba el cliente en el socket
	char *parse;
	char archivo[256];
	char bufferoriginal[4096];//Para copiar el contenido del buffer y mantener el original. Tiene el mismo tamaño que el bufer original

	strcpy(bufferoriginal, buffer);//Copia el contenido del buffer a bufferoriginal para analizarlo luego ya que strtok destruye el contenido original del buffer


	linea = strtok(buffer, "\n");//esto reemplazará el primer \n que encuentre por un \0 así trabajaremos unicamente con la primera linea
	if(linea == NULL){
		printf("No se encontro un salto de linea en el buffer\n");
		return -1;
	}

	printf("ANALIZANDO PETICIÓN...:\n");
	//comprobamos que sea una petición HTTP
	parse = strstr(linea," HTTP/");//Busca en la primera linea si existe " HTTP/" y devuelve un puntero a la posicion donde comienza
	if(parse == NULL){
		printf("La petición no es HTTP.\n");
	}else{
		*parse='\0'; //Colocamos un '\0' para en la posición que señala parse. (este '\0' queda inmediatamente despues de la pagina solicitada ej: index.html\0
		parse=NULL; //Volvemos el puntero a NULL
		parse = strtok(linea, " ");//Parsea el método
		strcpy(http->metodo,parse);//Lo guarda en la estructura
		parse = strtok(NULL, " ");//Parsea el archivo
		//Si no hay nombre de pagina, indicamos por defecto que es index.html
		if( strlen(parse) == 1){ //Si la petición solo pide "/" lo reemplazamos por un /index.html
			strcat(parse,"index.html");
		}
		strcpy(http->file,parse);//Lo guarda en la estructura
		strcpy(archivo,http->file);//Copia el archivo a una cadena de caracteres nueva así strtok no destruye el de la estructura
		parse = get_filename_ext(archivo);//Se parsea la extension
		http->mime = mime_gettype(parse);//Se analiza la extensión y se guarda en la estructura el mime type

		//Si el método es post, parseamos los datos json que contiene.
		if(strcmp(http->metodo, "POST") == 0){//Si el metodo es POST
			if ( strcmp(http->file, "/comando.json") == 0 ){ //Y la petición es un comando
				parse = strstr(bufferoriginal,"comando_xbee");//Busca en la petición si existe "comando_xbee" y devuelve un puntero a la posicion donde comienza
				if(linea == NULL){
					printf("No se encontro un la variable \"comando_xbee\" en los datos POST.\n");
					return -1;
				}
				parse = strchr(parse,'=')+1;//Devuelve un puntero al espacio siguiente del primer "=" que encuentre (que es el texto json a analizar)
				urldecode(http->data, parse);//Desde el cliente los datos json se envían codeados en html (un "{" se manda como "%7B"), con esta función los datos se guardan decodificados
			}

		}
	}

	return 0;
}

//Escribe una respuesta en el socket en base al contenido de la estructura http_t
int http_send_resp(int sd, http_t *http, char *dirraiz, struct xbee *xbee) {

	json_t *jasonData, *remotoData, *pinData, *comandoData; //Variables donde se guardarán los objetos json parseados
	json_error_t error; //Variable donde se guardarán errores del parseador en caso de producirse
	const char *remoto_valor, *pin_valor, *comando_valor; //Variables donde se guardarán los valores de lo recibido por json
	char remoto[50], pin[50], comando[50];
	int hayerror = 0;

	printf("Metodo: %s\n",http->metodo);
	printf("File: %s\n",http->file);
	printf("Mime/Type: %s\n",http->mime);
	if(strcmp(http->metodo, "POST") == 0){
		printf("Datos Post: %s\n",http->data);
	}


	char dirarchivo[1024];
	char respuesta[1024];
	int tamarch, leido, fd;

	if ( (strcmp(http->metodo, "NA") == 0) && (strcmp(http->file, "NA") == 0) && (strcmp(http->mime, "NA") == 0)){ //Si nunca se setearon las variables en la estructura http
		printf("Petición Inválida.\n");
		escribirensocket(sd,"Petición Inválida.\r\n");
		return 0;
	}

	if ( (strcmp(http->metodo, "GET")) && (strcmp(http->metodo, "POST")) ){ //Si el metodo no es ni get ni post
		printf("Error 501 Metodo HTTP no implementado.\n");
		escribirensocket(sd,"HTTP/1.0 501 Not Implemented\r\n");
		escribirensocket(sd,"Content-Type: text/html\r\n\r\n");
		escribirensocket(sd,"<html><head><title>Error 501</title></head>");
		escribirensocket(sd,"<body><h1>Error 501</h1><p>Metodo HTTP no implementado en el servidor.</p><h3>Compu II Server</h3></body></html>");
		return 0;
	}

	if ( strcmp(http->metodo, "POST") == 0 ){ //Si el metodo es POST
		printf("Respondiendo POST.\n");
		if ( strcmp(http->file, "/comando.json") == 0 ){ //Y la petición es un comando
			jasonData = json_loads(http->data, 0, &error);
			//Comprobamos que se haya parseado correctamente
			if(!jasonData){
				fprintf(stderr, "ERROR AL PARSEAR JSON. LÍNEA %d: %s\n", error.line, error.text);
				return -1;
			}
			//Obtenemos los valores de la estructura json y los guardamos en las variables correspondientes
			remotoData = json_object_get(jasonData, "remoto");
			remoto_valor = json_string_value(remotoData);
			strcpy(remoto, remoto_valor);
			pinData = json_object_get(jasonData, "pin");
			pin_valor = json_string_value(pinData);
			strcpy(pin, pin_valor);
			comandoData = json_object_get(jasonData, "comando");
			comando_valor = json_string_value(comandoData);
			strcpy(comando, comando_valor);

			//Ejecutamos el comando
			printf("EJECUTANDO COMANDO: Remoto: %s, Pin: %s, Comando: %s\n", remoto, pin, comando);
			if(ejecutarComando(xbee, remoto, pin, *comando) != 0){
				fprintf(stderr, "ERROR AL INTENTAR EJECUTAR COMANDO\n");
				hayerror = 1;
			}


		}

		escribirensocket(sd,"HTTP/1.0 200\r\n");
		escribirensocket(sd,"Content-Type: application/json\r\n\r\n");
		if(hayerror){
			escribirensocket(sd,"BAD\r\n");
		}else{
			escribirensocket(sd,"OK\r\n");
		}
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
		escribirensocket(sd,"<html><head><title>Error 404</title></head>");
		escribirensocket(sd,"<body><h1>Error 404</h1><p>Pagina no encontrada :(</p><h3>Compu II Server</h3></body></html>");
		return 0;
	}else{ //Si el archivo existe
		printf("200 OK! respondiendo peticion para un archivo %s\n", http->file);
		//Obtenemos el tamaño del archivo solicitado
		if( (tamarch = get_tamanioarchivo(fd)) == -1 ){
			printf("Error al obtener el tamaño del archivo.\n");
			return -1;
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


