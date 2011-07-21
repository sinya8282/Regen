REGENFLAGS=-D__ENABLE_XBYAK__=1 #to set 0, do compile without xbyak (JIT library).
CFLAGS=-O3 -Wall -g3 -fno-operator-names $(REGENFLAGS)
CC=g++

regen: regen.cc regex.o expr.o exprutil.o dfa.o generator.o test.o bench.o Makefile
	$(CC) $(CFLAGS) regen.cc regex.o expr.o exprutil.o dfa.o generator.o -o regen

test.o: test.cc regex.o 
	$(CC) $(CFLAGS) test.cc regex.o expr.o exprutil.o dfa.o generator.o -o test

bench.o: bench.cc regex.o
	$(CC) $(CFLAGS) bench.cc regex.o expr.o exprutil.o dfa.o generator.o -o bench

regex.o: regex.cc regex.h Makefile
	$(CC) $(CFLAGS) -c regex.cc -o regex.o

expr.o: expr.cc expr.h Makefile
	$(CC) $(CFLAGS) -c expr.cc -o expr.o

exprutil.o: exprutil.cc exprutil.h Makefile
	$(CC) $(CFLAGS) -c exprutil.cc -o exprutil.o

dfa.o: dfa.cc dfa.h Makefile
	$(CC) $(CFLAGS) -c dfa.cc -o dfa.o

generator.o: generator.cc generator.h Makefile
	$(CC) $(CFLAGS) -c generator.cc -o generator.o

paralleldfa.o: paralleldfa.cc paralleldfa.h dfa.o Makefile
	$(CC) $(CFLAGS) -c paralleldfa.cc paralleldfa.h

clean:
	rm -f *.o 