#ifndef _HILOS_H_
	#define _HILOS_H_

	//Estructura para mandar argumentos a los hilos nuevos
	typedef struct {
		int sd;//Puntero para hacer malloc. (Descriptor de socket)
		struct xbee *xbee; //Descriptor a xbee conectado al servidor
		char *dirraiz;//Directorio raiz
	} arg_struct;

	void* atenderconexion (void*);
	void* escucharCola (void*);

#endif
