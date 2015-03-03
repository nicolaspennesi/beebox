#include <string.h> //Para todos los str
#include "mime.h"

char* mime_gettype(char* extension)
{
	if (!strcmp(extension, "html")){
		return "text/html";
	}
	if (!strcmp(extension, "htm")){
		return "text/html";
	}
	if (!strcmp(extension, "jpg")){
		return "image/jpeg";
	}

	if (!strcmp(extension, "png")){
		return "image/png";
	}
	if (!strcmp(extension, "txt")){
		return "text/plain";
	}
	if (!strcmp(extension, "pdf")){
		return "application/pdf";
	}
	if (!strcmp(extension, "js")){
		return "text/javascript";
	}
	//Si no es ningun tipo de archivo conocido devuelve aplication/mime para que el navegador lo descargue
	return "aplication/mime";

}
