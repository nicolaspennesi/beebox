#include <xbee.h> // Librería para manejo interno de Xbees

#ifndef _HTTP_H_
	#define _HTTP_H_

	typedef struct http {
		char metodo[10];
		char file[256];
		char *mime;
		char data[4096];
	} http_t;

	int http_parse_req(char*, int, http_t*);
	int http_send_resp(int, http_t*, char*, struct xbee*);
	void escribirensocket(int,char*);
	int get_tamanioarchivo(int);
	void urldecode(char*, const char*);

#endif
