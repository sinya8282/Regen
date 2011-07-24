ENABLE_XBYAK=yes
ENABLE_PARALLEL=yes

ifeq ($(ENABLE_XBYAK),yes)
REGENFLAGS=-D__ENABLE_XBYAK__
endif
ifeq ($(ENABLE_PARALLEL),yes)
REGENFLAGS+=-D__ENABLE_PARALLEL__ -lboost_thread-mt
endif

CC=g++
CFLAGS=-O3 -Wall -g3 -fno-operator-names $(REGENFLAGS)

OBJS=regex.o expr.o exprutil.o dfa.o paralleldfa.o generator.o 

all: regen test.o bench.o

regen: regen.cc $(OBJS)
	$(CC) $(CFLAGS) regen.cc $(OBJS) -o regen

test: test.o
	@./test.o

bench: bench.o
	@./bench.o

test.o: test.cc $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) test.cc -o test.o

bench.o: bench.cc $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) bench.cc -o bench.o

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -f $(OBJS) test.o bench.o paralleldfa.h.gch