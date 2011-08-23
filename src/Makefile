REGEN_ENABLE_XBYAK=yes
REGEN_ENABLE_PARALLEL=yes

ifeq ($(REGEN_ENABLE_XBYAK),yes)
REGENFLAGS=-DREGEN_ENABLE_XBYAK
endif
ifeq ($(REGEN_ENABLE_PARALLEL),yes)
REGENFLAGS+=-DREGEN_ENABLE_PARALLEL
LIBTHREAD=-lboost_thread-mt
endif

CC=g++
CFLAGS=-O3 -Wall -g3 -fno-operator-names $(REGENFLAGS)
SRC=regex.cc expr.cc exprutil.cc dfa.cc ssfa.cc generator.cc 
OBJS=$(SRC:.cc=.o)

all: regen fullmatch state_num test.o bench.o

regen: regen.cc $(OBJS)
	$(CC) $(CFLAGS) $(LIBTHREAD) regen.cc $(OBJS) -o $@

fullmatch: tests/fullmatch.cc $(OBJS)
	$(CC) $(CFLAGS) $(LIBTHREAD) tests/fullmatch.cc $(OBJS) -o $@

state_num: tests/state_num.cc $(OBJS)
	$(CC) $(CFLAGS) $(LIBTHREAD) tests/state_num.cc $(OBJS) -o $@

test: test.o
	@./test.o

bench: bench.o
	@./bench.o

test.o: tests/test.cc $(OBJS)
	$(CC) $(CFLAGS) $(LIBTHREAD) tests/test.cc $(OBJS) -o test.o

bench.o: tests/bench.cc $(OBJS)
	$(CC) $(CFLAGS) $(LIBTHREAD) tests/bench.cc $(OBJS) -o bench.o 

.cc.o:
	$(CC) $(CFLAGS) -c $<

clean:
	rm -rf $(OBJS) test.o bench.o ssfa.h.gch regen test bench fullmatch state_num *.dSYM

depend:
	sed '/^# DO NOT DELETE THIS LINE/q' Makefile > tmp
	g++ -MM $(CFLAGS) $(SRC) >> tmp
	mv tmp Makefile

# DO NOT DELETE THIS LINE -- make depend depends on it.
regex.o: regex.cc regex.h util.h expr.h exprutil.h dfa.h ssfa.h
expr.o: expr.cc expr.h util.h
exprutil.o: exprutil.cc exprutil.h expr.h util.h
dfa.o: dfa.cc dfa.h util.h
ssfa.o: ssfa.cc ssfa.h regex.h util.h expr.h \
  exprutil.h dfa.h
generator.o: generator.cc generator.h regex.h util.h expr.h exprutil.h \
  dfa.h ssfa.h
