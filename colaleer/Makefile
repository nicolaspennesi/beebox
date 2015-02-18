CC=gcc
CFLAGS=-lrt -ljansson
TARGETS=beeboxmqreceive

all: $(TARGETS)

beeboxmqreceive: main.c
	$(CC) $^ $(CFLAGS) -o $@
#$@ reemplaza por beeboxmqreceive
#$^ reemplaza por las dependencias beeboxmqreceive:

test: beeboxmqreceive
	./beeboxmqreceive

clean:
	rm $(TARGETS)
#borra el valor de la variable $all

