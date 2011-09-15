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
SRC=regex.cc lexer.cc expr.cc exprutil.cc nfa.cc dfa.cc ssfa.cc generator.cc 
OBJS=$(SRC:.cc=.o)

all: regen fullmatch state_num test.o bench.o

regen: regen.cc $(OBJS)
	$(CC) regen.cc $(OBJS) -o $@ $(CFLAGS) $(LIBTHREAD)

fullmatch: tests/fullmatch.cc $(OBJS)
	$(CC) tests/fullmatch.cc $(OBJS) -o $@ $(CFLAGS) $(LIBTHREAD)

state_num: tests/state_num.cc $(OBJS)
	$(CC) tests/state_num.cc $(OBJS) -o $@ $(CFLAGS) $(LIBTHREAD)

test: test.o
	@./test.o

bench: bench.o
	@./bench.o

test.o: tests/test.cc $(OBJS)
	$(CC) tests/test.cc $(OBJS) -o test.o $(CFLAGS) $(LIBTHREAD)

bench.o: tests/bench.cc $(OBJS)
	$(CC) tests/bench.cc $(OBJS) -o bench.o $(CFLAGS) $(LIBTHREAD)

.cc.o:
	$(CC) -c $< $(CFLAGS)

clean:
	rm -rf $(OBJS) test.o bench.o ssfa.h.gch regen test bench fullmatch state_num *.dSYM

depend:
	sed '/^# DO NOT DELETE THIS LINE/q' Makefile > tmp
	g++ -MM $(CFLAGS) $(SRC) >> tmp
	mv tmp Makefile

# DO NOT DELETE THIS LINE -- make depend depends on it.
regex.o: regex.cc regex.h util.h lexer.h expr.h exprutil.h nfa.h dfa.h \
  ssfa.h
lexer.o: lexer.cc lexer.h util.h
expr.o: expr.cc expr.h util.h
exprutil.o: exprutil.cc exprutil.h expr.h util.h
nfa.o: nfa.cc nfa.h util.h
dfa.o: dfa.cc dfa.h util.h nfa.h expr.h
ssfa.o: ssfa.cc ssfa.h regex.h util.h lexer.h expr.h exprutil.h nfa.h \
  dfa.h
generator.o: generator.cc generator.h regex.h util.h lexer.h expr.h \
  exprutil.h nfa.h dfa.h ssfa.h
