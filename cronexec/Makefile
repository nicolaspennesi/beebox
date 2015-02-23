CC=gcc
CFLAGS=
TARGETS=cron

all: $(TARGETS)

cron: main.c
	$(CC) $^ $(CFLAGS) -o $@
#$@ reemplaza por cron
#$^ reemplaza por las dependencias cron:

test: cron
	./cron

clean:
	rm $(TARGETS)
#borra el valor de la variable $all

