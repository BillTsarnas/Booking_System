all: SERVEROS CLIENTOS
	./SERVEROS

CLIENTOS: clientos.c
	gcc clientos.c -o CLIENTOS

SERVEROS: functions.o serveros.o
	gcc serveros.o functions.o -pthread -o SERVEROS

functions.o: functions.c headeros.h
	gcc -Wall -c functions.c

serveros.o: serveros.c headeros.h
	gcc -Wall -c serveros.c

clean:
	rm -rf *o SERVEROS CLIENTOS socketfile
