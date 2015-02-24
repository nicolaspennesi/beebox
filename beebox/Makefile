CC=gcc
CFLAGS=-Wall -lrt -pthread -ljansson -lxbee
TARGETS=beebox
all: $(TARGETS)

beebox: main.c http.c mime.c escribirensocket.c tamanioarchivo.c urldecode.c xbee.c stringahex.c cron.c hilos.c
	$(CC) $^ $(CFLAGS) -o $@
#$@ reemplaza por beebox
#$^ reemplaza por las dependencias beebox:

test: beebox
	./beebox

clean:
	rm $(TARGETS)
#borra el valor de la variable $all

