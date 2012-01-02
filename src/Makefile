REGEN_ENABLE_XBYAK=yes
REGEN_ENABLE_PARALLEL=yes

ifeq ($(REGEN_ENABLE_XBYAK),yes)
REGENFLAGS=-DREGEN_ENABLE_XBYAK
endif
ifeq ($(REGEN_ENABLE_PARALLEL),yes)
REGENFLAGS+=-DREGEN_ENABLE_PARALLEL
LIBTHREAD=-lboost_thread-mt
SRC=regen.cc regex.cc lexer.cc expr.cc exprutil.cc nfa.cc dfa.cc ssfa.cc generator.cc
else
SRC=regen.cc regex.cc lexer.cc expr.cc exprutil.cc nfa.cc dfa.cc generator.cc
endif

ifeq ($(shell uname),Darwin)
MAKE_SHARED_LIBRARY=-dynamiclib -exported_symbols_list libregen.symbols.darwin
else
MAKE_SHARED_LIBRARY=-shared -Wl,-soname,libregen.so,--version-script=libregen.symbols
endif

CC=g++
CFLAGS=-fPIC -O3 -Wall -g3 -fno-operator-names $(REGENFLAGS)
OBJS=$(SRC:.cc=.o)

all: libregen recon fullmatch state_num regengrep test.o bench.o

libregen: $(OBJS)
	$(CC) $(OBJS) $(MAKE_SHARED_LIBRARY) -o libregen.so $(CFLAGS) $(LIBTHREAD)

recon: recon.cc $(OBJS)
	$(CC) recon.cc $(OBJS) -o $@ $(CFLAGS) $(LIBTHREAD)

fullmatch: tests/fullmatch.cc libregen
	$(CC) tests/fullmatch.cc -o $@ $(CFLAGS) $(LIBTHREAD) -L$(PWD) -lregen

state_num: tests/state_num.cc libregen
	$(CC) tests/state_num.cc -o $@ $(CFLAGS) $(LIBTHREAD) -L$(PWD) -lregen

regengrep: tests/regengrep.cc libregen
	$(CC) tests/regengrep.cc -o $@ $(CFLAGS) $(LIBTHREAD) -L$(PWD) -lregen

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
	rm -rf $(OBJS) test.o bench.o ssfa.h.gch recon test bench fullmatch state_num regengrep *.dSYM libregen.so

depend:
	sed '/^# DO NOT DELETE THIS LINE/q' Makefile > tmp
	g++ -MM $(CFLAGS) $(SRC) >> tmp
	mv tmp Makefile

# DO NOT DELETE THIS LINE -- make depend depends on it.
regen.o: regen.cc regen.h regex.h util.h lexer.h expr.h exprutil.h nfa.h \
  dfa.h ssfa.h
regex.o: regex.cc regex.h regen.h util.h lexer.h expr.h exprutil.h nfa.h \
  dfa.h ssfa.h
lexer.o: lexer.cc lexer.h util.h regen.h
expr.o: expr.cc expr.h util.h
exprutil.o: exprutil.cc exprutil.h expr.h util.h
nfa.o: nfa.cc nfa.h util.h
dfa.o: dfa.cc dfa.h regen.h util.h nfa.h expr.h
ssfa.o: ssfa.cc ssfa.h regen.h regex.h util.h lexer.h expr.h exprutil.h \
  nfa.h dfa.h
generator.o: generator.cc generator.h regex.h regen.h util.h lexer.h \
  expr.h exprutil.h nfa.h dfa.h ssfa.h
