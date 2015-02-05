/*
	Consulta el estado de un pin de un nodo remoto y lo cambia
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <xbee.h>

//Función callback, se ejecuta cuando el xbee coordinador recibe respuesta de un nodo.
void cambiar(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {

	printf("ENTRO AL CALLBACK\n");

	//Muestra respuesta del nodo (0 es OK)
	printf("Nodo Remoto Dice: %d\n",*(*pkt)->data);

	//Imprime OK cuando termina
	printf("OK\n");

}

//Función callback, se ejecuta cuando el xbee coordinador recibe respuesta de un nodo.
void evaluar(struct xbee *xbee, struct xbee_con *con, struct xbee_pkt **pkt, void **data) {

	unsigned char txErr;
	xbee_err err;

	//Imprime OK cuando termina
	printf("EVALUANDO..\n");


	//Setea la función callback
	if ((err = xbee_conCallbackSet(con, cambiar, NULL)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conCallbackSet() returned: %d", err);
	}

	//Si el pin está en bajo, lo pone en alto
	if ( *(*pkt)->data == 4 ) { //USAR MACROS #DEFINE
		err = xbee_conTx(con, &txErr, "D0%c", 0x05);
		printf("Nodo Local Dice: %d\n", err);
		if (err) {
			printf("txErr: %d\n", txErr);
		} else {
			//Tiempo de espera para respuesta
			usleep(1000000);
		}
	//Si el pin está en alto, lo pone en bajo
	}else if (*(*pkt)->data == 5){
		err = xbee_conTx(con, &txErr, "D0%c", 0x04);
		printf("Nodo Local Dice: %d\n", err);
		if (err) {
			printf("txErr: %d\n", txErr);
		} else {
			//Tiempo de espera para respuesta
			usleep(1000000);
		}
	}

}

int main(void) {
	void *d;
	struct xbee *xbee;
	struct xbee_con *con;
	struct xbee_conAddress address;
	unsigned char txErr;
	xbee_err err;

	//Setea la conexión con el xbee coordinador por puerto serie
	if ((err = xbee_setup(&xbee, "xbeeZB", "/dev/ttyUSB0", 9600)) != XBEE_ENONE) {
		printf("err: %d (%s)\n", err, xbee_errorToStr(err)); //Usar perror para errores (ver si se puede usar con libxbee)
		return err;
	}

	//Setea los datos del nodo remoto al que se le van a enviar las consultas
	/*
	* this is the 64-bit address of the remote XBee module
	* it should be entered with the MSB first, so the address below is
	* SH = 0x0013A200    SL = 0x40081826
	*/
	memset(&address, 0, sizeof(address));
	address.addr64_enabled = 1;
	address.addr64[0] = 0x00;
	address.addr64[1] = 0x13;
	address.addr64[2] = 0xA2;
	address.addr64[3] = 0x00;
	address.addr64[4] = 0x40;
	address.addr64[5] = 0x86;
	address.addr64[6] = 0x92;
	address.addr64[7] = 0xB8;

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
	printf("Nodo Local Dice: %d\n", err);
	if (err) {
		printf("txErr: %d\n", txErr);
	} else {
		//Tiempo de espera para respuesta //Sacar usleep y bloquear hasta que llegue la respuesta
		usleep(1000000);
	}

	//Espera la respuesta del nodo así tiene tiempo de entrar en la funcion callback, en el programa final como esto es un bucle infinito no hace falta
	sleep(5);

	//Cierra la conexión con el nodo remoto
	if ((err = xbee_conEnd(con)) != XBEE_ENONE) {
		xbee_log(xbee, -1, "xbee_conEnd() returned: %d", err);
		return err;
	}

	//Cierra conexión por puerto serie con el coordinador
	xbee_shutdown(xbee);

	return 0;
}
