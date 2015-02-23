//------------------
//-- LIBRERIAS
//------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>//Para el open
#include <errno.h> //Para mostrar errores
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

sem_t semaforo_xbee; //Declaramos la vaiable en la que estaá el semaforo
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER; //Inicializo una variable mutex en una sola linea


//------------------
//-- FUNCIONES
//------------------
int manejarError(struct xbee_con *con, int error){
	xbee_err err;
	//Cierra la conexión con el nodo remoto
	if ((err = xbee_conEnd(con)) != XBEE_ENONE) {
		fprintf(stderr, "xbee_conEnd() returned: %d\n", err);
		return err;
	}
	//Desbloquea el mutex
	printf("LIBERO MUTEX\n");
	pthread_mutex_unlock (&mutex);
	return error;
}

void manejadorErroresCoordinador(xbee_err res, unsigned char resval){
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
void analizarRespuestaNodoRemoto(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {

	printf("\nCALLBACK: ANALIZANDO RESPUESTA\n");

	//Muestra respuesta del nodo (0 es OK)
	switch (*(*pkt)->data){
		case 0: //Si el pin está en bajo, lo pone en alto
			printf("NODO REMOTO DICE: OK\n");
			break;
		default:
			fprintf(stderr, "NODO REMOTO DICE: ERROR (0x%02X)\n", *(*pkt)->data);
			break;
	}

	printf("LIBERO SEMÁFORO...\n");
	sem_post(&semaforo_xbee);//Sumo 1 en el semaforo

}

//Función callback, se ejecuta cuando el xbee coordinador recibe respuesta de un nodo.
void evaluaryCambiar(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {

	unsigned char txErr;
	xbee_err err;
	unsigned char *pinConsultado = (*pkt)->atCommand; //En (*pkt)->atCommand se encuentra el pin por el cual se consultó

	printf("\nCALLBACK: EVALUANDO ESTADO..\n");

	//Setea la función callback
	if ((err = xbee_conCallbackSet(con, analizarRespuestaNodoRemoto, NULL)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", err);
	}

	//Analiza la respuesta de la consulta del estado del pin
	switch (*(*pkt)->data){
		case BAJO: //Si el pin está en bajo, lo pone en alto
			printf("ESTADO: APAGADO\n");
			err = xbee_conTx(con, &txErr, "%s%c", pinConsultado, ALTO);
			printf("ENVIANDO COMANDO ENCENDER...\n");
			break;
		case ALTO: //Si el pin está en alto, lo pone en bajo
			printf("ESTADO: ENCENDIDO\n");
			err = xbee_conTx(con, &txErr, "%s%c", pinConsultado, BAJO);
			printf("ENVIANDO COMANDO APAGAR...\n");
			break;
		default:
			fprintf(stderr, "ERROR AL INTERPRETAR RESPUESTA DEL NODO REMOTO (0x%02X)\n", *(*pkt)->data);
			return;
			break;
	}

	//Evalúa si hubo algún error al enviar el comando
	if (err != XBEE_ENONE) {
		manejadorErroresCoordinador(err, txErr);
	} else{
		printf("COMANDO ENVIADO\n");
	}

}

int ejecutarComando(struct xbee *xbee, char *remoteaddr, char *pinstring, char comando){

	int pinint;
	char *pin;

	struct xbee_con *con; //Descriptor al tipo de conexión entre el xbee remoto y el coordinador
	struct xbee_conAddress address; //Dirección del xbee remoto
	unsigned char txErr; //Variable en donde se almacena un caracter en caso de errror
	xbee_err err; //Variable para guardar errores

	sem_init(&semaforo_xbee,0, 0); //Inicializamos el semáforo. EL segundo argumento 0 indica que es un semaforo no compartido entre los procesos relacionados. El tercer argumento 0 indica el valor inicial
	struct timespec ts; //Variable para setear el timeout del semáforo
	int retsemaforo; //Variable para guardar el resultado del semáforo (y ver si terminó bien o por timeout)

	//Setea los datos del nodo remoto al que se le van a enviar las consultas
	memset(&address, 0, sizeof(address));
	address.addr64_enabled = 1;
	stringahex(remoteaddr, address.addr64); //Convierte a hexadecimal la cadena que indica el id del nodo remoto

	//Bloquea el mutex
	printf("BLOQUEO MUTEX\n");
	pthread_mutex_lock (&mutex);

	//Setea el tipo de conexión que tendrá el coordinador con el nodo remoto, (en este caso será un Comando AT Remoto) y lo almacena en la variable 'con'
	if ((err = xbee_conNew(xbee, &con, "Remote AT", &address)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conNew() returned: %d (%s)", err, xbee_errorToStr(err));
		return err;
	}

	//Prepara el código del pin
	pinint = atoi(pinstring);
	switch (pinint){
		case 1:
			pin = PIN1;
			break;
		case 2:
			pin = PIN2;
			break;
		case 3:
			pin = PIN3;
			break;
		case 4:
			pin = PIN4;
			break;
		default:
			fprintf(stderr, "EL PIN SOLO PUEDE SER 1, 2, 3 o 4\n");
			return manejarError(con, -1);
			break;
	}

	//Evalúa el comando
	switch (comando){
		case 'c': //En caso de que sea cambiar (de estado)
			//Setea la función callback
			if ((err = xbee_conCallbackSet(con, evaluaryCambiar, NULL)) != XBEE_ENONE) {
				xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", err);
				return manejarError(con, err);
			}
			//Envía consulta al nodo remoto (pregunta por estado de del pin)
			err = xbee_conTx(con, &txErr, pin);
			printf("ENVIANDO COMANDO CAMBIAR ESTADO...\n");
			break;
		case 'e': //En caso de que sea encender
			//Setea la función callback
			if ((err = xbee_conCallbackSet(con, analizarRespuestaNodoRemoto, NULL)) != XBEE_ENONE) {
				xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", err);
				return manejarError(con, err);
			}
			//Envía consulta al nodo remoto (pregunta por estado de del pin)
			err = xbee_conTx(con, &txErr, "%s%c", pin, ALTO);
			printf("ENVIANDO COMANDO ENCENDER...\n");
			break;

		case 'a': //En caso de que sea apagar
			//Setea la función callback
			if ((err = xbee_conCallbackSet(con, analizarRespuestaNodoRemoto, NULL)) != XBEE_ENONE) {
				xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", err);
				return manejarError(con, err);
			}
			//Envía consulta al nodo remoto (pregunta por estado de del pin)
			err = xbee_conTx(con, &txErr, "%s%c", pin, BAJO);
			printf("ENVIANDO COMANDO APAGAR...\n");
			break;
	}

	//Evalúa si hubo algún error al enviar el comando
	if (err != XBEE_ENONE) {
		manejadorErroresCoordinador(err, txErr);
		return manejarError(con, err);
	} else{
		printf("COMANDO ENVIADO\n");
		//Queda esperando respuesta del nodo así tiene tiempo de entrar en la funcion callback
		printf("BLOQUEO SEMÁFORO...\n");
		//Seteo el timeout del semáforo a 5 segundos de la hora actual
		clock_gettime(CLOCK_REALTIME, &ts);
		ts.tv_sec += 5;
		retsemaforo = sem_timedwait(&semaforo_xbee, &ts);//Resto 1 en el semaforo (y queda esperando a estar nuevamente en un valor positivo para seguir)

		//Chequeo qué pasó con el semáforo en caso de error o timeout
		if (retsemaforo == -1) {
			if (errno == ETIMEDOUT){
				printf("TIMEOUT DEL SEMAFORO CUMPLIDO...\n");
				return manejarError(con, -1);
			}else{
				fprintf(stderr, "ERROR AL INICIAR SEMÁFORO\n");
			}
		}
	}

	//Cierra la conexión con el nodo remoto
	if ((err = xbee_conEnd(con)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conEnd() returned: %d", err);
		return err;
	}
	//Desbloquea el mutex
	printf("LIBERO MUTEX\n");
	pthread_mutex_unlock (&mutex);

	return 0;

}