CC=gcc
CFLAGS=-Wall -lrt -pthread
TARGETS=server
all: $(TARGETS)

server: server.c http.c mime.c escribirensocket.c tamanioarchivo.c
	$(CC) $^ $(CFLAGS) -o $@
#$@ reemplaza por server
#$^ reemplaza por las dependencias server:

test: server
	./server

clean:
	rm $(TARGETS)
#borra el valor de la variable $all

