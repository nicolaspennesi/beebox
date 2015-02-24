//------------------
//-- LIBRERIAS
//------------------

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
#include "cron.h" //Funciones propias

//------------------
//-- CONSTANTES
//------------------

extern char *dirbeeboxmqsend; //Ruta donde está el ejecutable que escribirá las tareas en la cola (para cron). Se obtiene del archivo main.c

//------------------
//-- FUNCIONES
//------------------

//Devuelve la extensión del nombre de un archivo
char *get_filename_ext(char *filename) {
	char *dot;

	dot = strrchr(filename, '.'); //Devuelve un puntero al último caracter especificado encontrado dentro de la cadena
	if(!dot || dot == filename) return "noext"; //Si no encontró ninguno devuelve una cadena vacía
	return dot + 1; //Si encontró devuelve un puntero al caracter siguiente (que sería la extensión)
}

//Pasea la petición HTTP y completa la estructura http_t con lo parseado
int http_parse_req(char *buffer, int leido, http_t* http){
	char *linea = NULL; //Contendrá la primera linea de lo que escriba el cliente en el socket
	char *parse;
	char archivo[256];
	char bufferoriginal[4096];//Para copiar el contenido del buffer y mantener el original. Tiene el mismo tamaño que el bufer original

	memset(bufferoriginal, 0, sizeof bufferoriginal); //Llena la variable bufferoriginal con 0
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
				if(parse == NULL){
					printf("No se encontro un la variable \"comando_xbee\" en los datos POST.\n");
					strcpy(http->data,"error");
					return -1;
				}
				parse = strchr(parse,'=')+1;//Devuelve un puntero al espacio siguiente del primer "=" que encuentre (que es el texto json a analizar)
				urldecode(http->data, parse);//Desde el cliente los datos json se envían codeados en html (un "{" se manda como "%7B"), con esta función los datos se guardan decodificados
			}else if ( strcmp(http->file, "/tarea.json") == 0 ){ //Y la petición es uns tarea
				printf("BUSCANDO\n");
				parse = strstr(bufferoriginal,"tarea_xbee");//Busca en la petición si existe "tarea_xbee" y devuelve un puntero a la posicion donde comienza
				printf("FIN BUSCANDO\n");
				if(parse == NULL){
					printf("No se encontro un la variable \"tarea_xbee\" en los datos POST.\n");
					strcpy(http->data,"error");
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

	json_t *jasonData, *remotoData, *pinData, *comandoData, *accionData, *minutoData, *horaData, *diaDelMesData, *mesData, *diaDeLaSemanaData; //Variables donde se guardarán los objetos json parseados
	json_error_t error; //Variable donde se guardarán errores del parseador en caso de producirse
	const char *remoto_valor, *pin_valor, *comando_valor, *accion_valor, *minuto_valor, *hora_valor, *diaDelMes_valor, *mes_valor, *diaDeLaSemana_valor; //Variables donde se guardarán los valores de lo recibido por json
	char remoto[20], pin[5], comando[10], accion[10], minuto[5], hora[5], diaDelMes[5], mes[5], diaDeLaSemana[5]; //Para evitar problema con tipo de variable "const"
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
		if ( strcmp(http->data, "error") == 0 ){ //Si no se encontraron los argumentos correctos (comando_xbee o tarea_xbee)
			escribirensocket(sd,"HTTP/1.0 200\r\n");
			escribirensocket(sd,"Content-Type: application/json\r\n\r\n");
			escribirensocket(sd,"Argumentos POST incorrectos.\r\n");
			return 0;
		}
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
				hayerror = 1;
			}

		}else if( strcmp(http->file, "/tarea.json") == 0 ){ //Si la petición es una tarea
			jasonData = json_loads(http->data, 0, &error);
			//Comprobamos que se haya parseado correctamente
			if(!jasonData){
				fprintf(stderr, "ERROR AL PARSEAR JSON. LÍNEA %d: %s\n", error.line, error.text);
				return -1;
			}
			//Obtenemos los valores de la estructura json y los guardamos en las variables correspondientes
			accionData = json_object_get(jasonData, "accion");
			accion_valor = json_string_value(accionData);
			strcpy(accion, accion_valor);
			minutoData = json_object_get(jasonData, "minuto");
			minuto_valor = json_string_value(minutoData);
			strcpy(minuto, minuto_valor);
			horaData = json_object_get(jasonData, "hora");
			hora_valor = json_string_value(horaData);
			strcpy(hora, hora_valor);
			diaDelMesData = json_object_get(jasonData, "diaDelMes");
			diaDelMes_valor = json_string_value(diaDelMesData);
			strcpy(diaDelMes, diaDelMes_valor);
			mesData = json_object_get(jasonData, "mes");
			mes_valor = json_string_value(mesData);
			strcpy(mes, mes_valor);
			diaDeLaSemanaData = json_object_get(jasonData, "diaDeLaSemana");
			diaDeLaSemana_valor = json_string_value(diaDeLaSemanaData);
			strcpy(diaDeLaSemana, diaDeLaSemana_valor);
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
			printf("TAREA: Minuto: %s, Hora: %s, Día del Mes: %s, Mes: %s, Día de la Semana: %s, Remoto: %s, Pin: %s, Comando: %s\n", minuto, hora, diaDelMes, mes, diaDeLaSemana, remoto, pin, comando);
			//Evaliamos si hay que agregar o quitar
			switch (*accion){ //evalúa el primer caracter del argumento de accion
				case 'a':
					if (agregarTareaCron(dirbeeboxmqsend, minuto, hora, diaDelMes, mes, diaDeLaSemana, remoto, comando, pin) != 0){
						hayerror = 1;
					}
					break;
				case 'q':
					if (quitarTareaCron(dirbeeboxmqsend, minuto, hora, diaDelMes, mes, diaDeLaSemana, remoto, comando, pin) != 0){
						hayerror = 1;
					}
					break;
				default:
					escribirensocket(sd,"HTTP/1.0 200\r\n");
					escribirensocket(sd,"Content-Type: application/json\r\n\r\n");
					escribirensocket(sd,"{\"error\":\"Accion incorrecta. [a=agregar | q=quitar]\"}");
					return 0;
			}

		}else{
			escribirensocket(sd,"HTTP/1.0 200\r\n");
			escribirensocket(sd,"Content-Type: application/json\r\n\r\n");
			escribirensocket(sd,"{\"error\":\"Petición POST a destino incorrecto. [/comando.json | /tarea.json]\"}");
			return 0;
		}

		escribirensocket(sd,"HTTP/1.0 200\r\n");
		escribirensocket(sd,"Content-Type: application/json\r\n\r\n");
		if(hayerror){
			escribirensocket(sd,"{\"content\":\"BAD\"}");
		}else{
			escribirensocket(sd,"{\"content\":\"OK\"}");
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


