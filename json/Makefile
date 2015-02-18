CC=gcc
CFLAGS=-ljansson
TARGETS=json

all: $(TARGETS)

json: main.c
	$(CC) $^ $(CFLAGS) -o $@
#$@ reemplaza por json
#$^ reemplaza por las dependencias json:

test: json
	./json

clean:
	rm $(TARGETS)
#borra el valor de la variable $all

