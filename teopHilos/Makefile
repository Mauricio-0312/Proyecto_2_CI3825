###########################
# Flags de la compilación #
###########################

CC=gcc

#########################
# Flags para debbugging #
########################

CFLAGS=-Wall -Wextra -Wdouble-promotion -Wno-unused-parameter  -Wno-unused-function -Wno-sign-conversion -Werror -fsanitize=undefined -std=gnu23 -O0 -ggdb


###############
# Librerias #
###############

LIBS=-lpthread

###############
# Compilación #
###############

PROG=teoph

ALL: $(PROG)

$(PROG): thread.o
	$(CC) $(CFLAGS) $(LIBS) -o $(PROG) thread.o

thread.o: thread.c
	$(CC) $(CFLAGS) -c thread.c

clean:
	rm -f teoph *.o *~
