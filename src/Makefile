CFLAGS=-O0 -g3 -fno-operator-names
CC=g++

regen: regex expr exprutil dfa generator
	$(CC) $(CFLAGS) regen.cc regex.o expr.o exprutil.o dfa.o generator.o -o regen

regex:
	$(CC) $(CFLAGS) -c regex.cc -o regex.o

expr:
	$(CC) $(CFLAGS) -c expr.cc -o expr.o

exprutil:
	$(CC) $(CFLAGS) -c exprutil.cc -o exprutil.o

dfa:
	$(CC) $(CFLAGS) -c dfa.cc -o dfa.o

generator:
	$(CC) $(CFLAGS) -c generator.cc -o generator.o

clean:
	rm -f *.o