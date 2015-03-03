#ifndef _HTTP_H_
	#define _HTTP_H_

	typedef struct http {
		char metodo[10];
		char file[256];
		char* mime;	
	} http_t;

	int http_parse_req(char*, int, http_t*, int);
	int http_send_resp(int, http_t*, char*);
	void escribirensocket(int,char*);
	int get_tamanioarchivo(int);

#endif
