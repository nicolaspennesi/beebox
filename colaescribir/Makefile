CC=gcc
CFLAGS=-lrt
TARGETS=beeboxmqsend

all: $(TARGETS)

beeboxmqsend: main.c
	$(CC) $^ $(CFLAGS) -o $@
#$@ reemplaza por beeboxmqsend
#$^ reemplaza por las dependencias beeboxmqsend:

test: beeboxmqsend
	./beeboxmqsend -r 0013A2004086A3FF -p 1 -c c

clean:
	rm $(TARGETS)
#borra el valor de la variable $all

