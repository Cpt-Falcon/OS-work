CC=gcc
#Using maximal error checking compiler flags including optimization flag
CFLAGS=-Wall -Werror -pedantic -O3

pipeit: pipeit.o
	$(CC) $(CFLAGS) pipeit.o -o pipeit
	touch *

#dependency of pipeit so that exectuable is generated from object file
pipeit.o:
	$(CC) $(CFLAGS) pipeit.c -c

clean:
	-rm *.o
	rm pipeit
	rm outfile