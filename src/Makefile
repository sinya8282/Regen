CFLAGS=-O0 -Wall -g3 -fno-operator-names
CC=g++

regen: regen.cc regex.o expr.o exprutil.o dfa.o generator.o test
	$(CC) $(CFLAGS) regen.cc regex.o expr.o exprutil.o dfa.o generator.o -o regen

test: test.cc regex.o expr.o exprutil.o dfa.o generator.o paralleldfa.o
	$(CC) $(CFLAGS) -L/usr/local/lib -lboost_thread-mt test.cc regex.o expr.o exprutil.o dfa.o generator.o paralleldfa.o -o test

regex.o: regex.cc regex.h
	$(CC) $(CFLAGS) -c regex.cc -o regex.o

expr.o: expr.cc expr.h
	$(CC) $(CFLAGS) -c expr.cc -o expr.o

exprutil.o: exprutil.cc exprutil.h
	$(CC) $(CFLAGS) -c exprutil.cc -o exprutil.o

dfa.o: dfa.cc dfa.h
	$(CC) $(CFLAGS) -c dfa.cc -o dfa.o

generator.o: generator.cc generator.h
	$(CC) $(CFLAGS) -c generator.cc -o generator.o

paralleldfa.o: paralleldfa.cc paralleldfa.h
	$(CC) $(CFLAGS) -c paralleldfa.cc paralleldfa.h

clean:
	rm -f *.o 