//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>//Para el open
#include <errno.h> //Para perror
#include <unistd.h> //Para opciones (argumentos) y otras cosas
#include <pthread.h>// Para hilos
#include <semaphore.h> //Para los semáforos
#include <time.h> //Para funciones de tiempo
#include <xbee.h> // Librería para manejo interno de Xbees
#include "utilidades.h" //Funciones propias

//------------------
//-- CONSTANTES
//------------------

#define PIN1 "D0" //Primer Digital IO del Xbee
#define PIN2 "D1" //Segundo Digital IO del Xbee
#define PIN3 "D2" //Tercer Digital IO del Xbee
#define PIN4 "D3" //Cuarto Digital IO del Xbee
#define ALTO 0x05 //El estado 5 de un pin de un xbee significa que está en high
#define BAJO 0x04 //El estado 5 de un pin de un xbee significa que está en high

//------------------
//-- VARIABLESGLOBALES
//------------------

sem_t semaforo; //Declaramos la vaiable en la que estaá el semaforo

void analizarespuesta(xbee_err res, unsigned char resval){
	if (res == XBEE_ETX) {
		if (resval == 4) {
			fprintf(stderr, "ERROR: NODO REMOTO NO ENCONTRADO\n");
		} else {
			fprintf(stderr, "ERROR DE TRANSMISIÓN (0x%02X)\n", resval);
		}
	} else {
		fprintf(stderr, "ERROR AL COMUNICARSE CON COORDINADOR: %s\n", xbee_errorToStr(res));
	}
}

//Función callback, se ejecuta cuando el xbee coordinador recibe respuesta de un nodo.
void cambiar(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {

	printf("CALLBACK 2\n");

	//Muestra respuesta del nodo (0 es OK)
	switch (*(*pkt)->data){
		case 0: //Si el pin está en bajo, lo pone en alto
			printf("NODO REMOTO DICE: OK");
			break;
		default:
			fprintf(stderr, "NODO REMOTO DICE: ERROR (0x%02X)\n", *(*pkt)->data);
			break;
	}

	printf("LIBERO SEMÁFORO...\n");
	sem_post(&semaforo);//Sumo 1 en el semaforo

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

	//Analiza la respuesta de la consulta del estado del pin
	switch (*(*pkt)->data){
		case BAJO: //Si el pin está en bajo, lo pone en alto
			printf("ESTADO: APAGADO\n");
			err = xbee_conTx(con, &txErr, "%s%c", PIN2, ALTO);
			if (err != XBEE_ENONE) {
				analizarespuesta(err, txErr);
			} else{
				printf("ENVIANDO COMANDO ENCENDER...\n");
			}
			break;
		case ALTO: //Si el pin está en alto, lo pone en bajo
			printf("ESTADO: ENCENDIDO\n");
			err = xbee_conTx(con, &txErr, "%s%c", PIN2, BAJO);
			if (err != XBEE_ENONE) {
				analizarespuesta(err, txErr);
			} else{
				printf("ENVIANDO COMANDO APAGAR...\n");
			}
			break;
		default:
			fprintf(stderr, "ERROR AL INTERPRETAR RESPUESTA DEL NODO REMOTO (0x%02X)\n", *(*pkt)->data);
			return;
			break;
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

	sem_init(&semaforo,0, 0); //Inicializamos el semáforo. EL segundo argumento 0 indica que es un semaforo no compartido entre los procesos relacionados. El tercer argumento 0 indica el valor inicial
	struct timespec ts;
	int retsemaforo;

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
		fprintf(stderr, "Error al abrir el archivo de configuración especificado");
		return -1;
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
		fprintf(stderr, "ERROR AL INTENTAR ABRIR CONEXION POR PUERTO SERIE: %d (%s)\n", err, xbee_errorToStr(err));
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

	//Envía consulta al nodo remoto (pregunta por estado de DEL PIN2)
	err = xbee_conTx(con, &txErr, PIN2);
	if (err != XBEE_ENONE) {
		analizarespuesta(err, txErr);
	} else{
		printf("COMANDO ENVIADO\n");
		//Queda esperando respuesta del nodo así tiene tiempo de entrar en la funcion callback
		printf("BLOQUEO SEMÁFORO...\n");
		//Seteo el timeout del semáforo a 5 segundos de la hora actual
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 5;
		retsemaforo = sem_timedwait(&semaforo, &ts);//Resto 1 en el semaforo (y queda esperando a estar nuevamente en un valor positivo para seguir)

		//Chequeo qué pasó con el semáforo en caso de error o timeout
		if (retsemaforo == -1) {
			if (errno == ETIMEDOUT)
				printf("TIMEOUT DEL SEMAFORO CUMPLIDO...\n");
			else
				fprintf(stderr, "ERROR AL INICIAR SEMÁFORO");
		}
	}

	printf("FIN MAIN\n");

	//Cierra la conexión con el nodo remoto
	if ((err = xbee_conEnd(con)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conEnd() returned: %d", err);
		return err;
	}

	//Cierra conexión por puerto serie con el coordinador
	xbee_shutdown(xbee);

	return 0;
}
