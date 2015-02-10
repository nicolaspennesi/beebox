//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>//Para el open
#include <errno.h> //Para perror
#include <unistd.h> //Para opciones (argumentos) y otras cosas

#include <xbee.h> // Librería para manejo interno de Xbees
#include "utilidades.h" //Funciones propias

//------------------
//-- CONSTANTES
//------------------

#define ALTO 0x05 //El estado 5 de un pin de un xbee significa que está en high
#define BAJO 0x04 //El estado 5 de un pin de un xbee significa que está en high


//Función callback, se ejecuta cuando el xbee coordinador recibe respuesta de un nodo.
void cambiar(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {

	printf("CALLBACK 2\n");

	//Muestra respuesta del nodo (0 es OK)
	printf("NODO REMOTO DICE: %d\n",*(*pkt)->data);

	//Imprime OK cuando termina
	printf("OK\n");

}

//Función callback, se ejecuta cuando el xbee coordinador recibe respuesta de un nodo.
void evaluar(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {

	unsigned char txErr;
	xbee_err err;

	printf("CALLBACK 1\n");
	printf("EVALUANDO..\n");

	//Setea la función callback
	if ((err = xbee_conCallbackSet(con, cambiar, NULL)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", err);
	}

	//Si el pin está en bajo, lo pone en alto
	if ( *(*pkt)->data == BAJO ) {
		printf("ESTADO: APAGADO\n");
		err = xbee_conTx(con, &txErr, "D0%c", ALTO);
		printf("NODO LOCAL DICE: %d\n", err);
		if (err) {
			printf("txErr: %d\n", txErr);
		} else {
			printf("ENVIANDO COMANDO ENCENDER...\n");
		}
	//Si el pin está en alto, lo pone en bajo
	}else if (*(*pkt)->data == ALTO){
		printf("ESTADO: ENCENDIDO\n");
		err = xbee_conTx(con, &txErr, "D0%c", BAJO);
		printf("NODO LOCAL DICE: %d\n", err);
		if (err) {
			printf("txErr: %d\n", txErr);
		} else {
			printf("ENVIANDO COMANDO APAGAR...\n");
		}
	}

}

int main(int argc, char *const argv[]){

	int opcion; //Para el manejo de argumentos

	char *archconfig = "default.cfg"; //Puntero al nombre del archivo de configuración. Por defecto es default.cfg

	char *remoteaddr = "0013A2004086A3FF";
	char *comando;
	char *puertoserie;
	char *baudiosstring;
	int baudios;

	int descarch; //Descriptor del archivo de configuracion
	int leido;
	char buffer[1024];

	void *d;
	struct xbee *xbee;
	struct xbee_con *con;
	struct xbee_conAddress address;
	unsigned char txErr;
	xbee_err err;

	//Se analizan los argumentos
	while ((opcion = getopt (argc, argv, "f:r:c:")) != -1){
		switch (opcion){
			case 'f':
				archconfig = optarg;
				break;
			case 'r':
				remoteaddr = optarg;
				break;
			case 'c':
				comando = optarg;
				break;
			case '?':
				if (optopt == 'f') {
					fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
				} else if (optopt == 'r') {
					fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
				} else if (optopt == 'c') {
					fprintf (stderr, "La opción -%c requiere un argumento.\n", optopt);
				} else if (isprint (optopt)) {
					fprintf (stderr, "Opción '-%c' desconocida.\n", optopt);
				}
				return 1;
			default:
				printf ("Argumentos Incorrectos");
				return 1;
		}
	}

	//Leemos el archivo de configuracion y guardamos sus datos en las variables globales correspondientes
	if ((descarch = open(archconfig,O_RDONLY,0)) < 0){
		printf("Error al abrir el archivo de configuración especificado");
		return 1;
	}
	leido = read (descarch, buffer, sizeof buffer);
	buffer[leido] = '\0'; //Agrega un \0 al final de lo que se guardó en el buffer así strtok trabaja correctamente
	//Parseamos para dejar en la variable puertoserie el puerto especificado en el archivo de configuracion (string)
	puertoserie = strtok (buffer, "=");
	puertoserie = strtok (NULL, "\n");
	//Parseamos para dejar en la variable baudiosstring los baudios especificados en el archivo de configuracion (en string)
	baudiosstring = strtok (NULL, "=");
	baudiosstring = strtok (NULL, "\n");
	baudios = atoi(baudiosstring); //transformamos el string a int y lo guardamos en la variable baudios

	//Setea la conexión con el xbee coordinador por puerto serie
	if ((err = xbee_setup(&xbee, "xbeeZB", puertoserie, baudios)) != XBEE_ENONE) {
		printf("err: %d (%s)\n", err, xbee_errorToStr(err)); //Usar perror para errores (ver si se puede usar con libxbee)
		return err;
	}
	printf ("Conexión establecida en %s a %d baudios.\n", puertoserie, baudios);


	//Setea los datos del nodo remoto al que se le van a enviar las consultas
	memset(&address, 0, sizeof(address));
	address.addr64_enabled = 1;
	stringahex(remoteaddr, address.addr64); //Convierte a hexadecimal la cadena que indica el id del nodo remoto

	//Setea el tipo de conexión que tendrá el coordinador con el nodo remoto, (en este caso será un Comando AT Remoto) y lo almacena en la variable 'con'
	if ((err = xbee_conNew(xbee, &con, "Remote AT", &address)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conNew() returned: %d (%s)", err, xbee_errorToStr(err));
		return err;
	}

	//Setea la función callback
	if ((err = xbee_conCallbackSet(con, evaluar, NULL)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", err);
		return err;
	}

	//Envía consulta al nodo remoto (pregunta por estado de D0)
	err = xbee_conTx(con, &txErr, "D0");
	printf("NODO LOCAL DICE: %d\n", err);
	if (err) {
		printf("txErr: %d\n", txErr);
	}

	//Espera la respuesta del nodo así tiene tiempo de entrar en la funcion callback, en el programa final como esto es un bucle infinito no hace falta
	printf("FIN MAIN, ESPERANDO 1 SEGUNDO...\n");
	sleep(1);

	//Cierra la conexión con el nodo remoto
	if ((err = xbee_conEnd(con)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conEnd() returned: %d", err);
		return err;
	}

	//Cierra conexión por puerto serie con el coordinador
	xbee_shutdown(xbee);

	return 0;
}
