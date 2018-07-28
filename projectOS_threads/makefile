all: SERVEROS CLIENTOS
	./SERVEROS
	chmod 755 run_tests.sh

CLIENTOS: clientos.c
	gcc clientos.c -o CLIENTOS

SERVEROS: functions2.o serveros2.o
	gcc serveros2.o functions2.o -pthread -o SERVEROS

functions.o: functions2.c headeros2.h
	gcc -Wall -c functions2.c

serveros.o: serveros2.c headeros2.h
	gcc -Wall -c serveros2.c

clean:
	rm -rf *o SERVEROS CLIENTOS socketfile
