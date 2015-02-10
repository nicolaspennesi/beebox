CC=gcc
CFLAGS=-lxbee
TARGETS=xbee

all: $(TARGETS)

xbee: main.c stringahex.c
	$(CC) $^ $(CFLAGS) -o $@
#$@ reemplaza por xbee
#$^ reemplaza por las dependencias xbee:

test: xbee
	./xbee

clean:
	rm $(TARGETS)
#borra el valor de la variable $all

