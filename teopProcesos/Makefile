#######################
# Makefile para teopp #
#######################


## Compilador ##

CC=gcc


## Flags para debbugging ##


CFLAGS=-Wall -Wextra -Wdouble-promotion -Wno-unused-parameter  -Wno-unused-function -Wno-sign-conversion -Werror -fsanitize=undefined -std=gnu23 -O0 -ggdb


## Librerias ##


#LIBS=-lpthread


## Compilación ##


PROG=teopp

ALL: $(PROG)

$(PROG): process_long.o
	$(CC) $(CFLAGS) $(LIBS) -o $(PROG) process_long.o

thread.o: process.c
	$(CC) $(CFLAGS) -c process_long.c


## Limpieza ##


clean:
	rm -f teopp *.o *~
