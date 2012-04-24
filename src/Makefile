REGEN_ENABLE_JIT=yes
REGEN_ENABLE_PARALLEL=yes

ifeq ($(REGEN_ENABLE_JIT),yes)
REGENFLAGS=-DREGEN_ENABLE_JIT
SRC_=jitter.cc
endif
ifeq ($(REGEN_ENABLE_PARALLEL),yes)
REGENFLAGS+=-DREGEN_ENABLE_PARALLEL
LIBTHREAD=-lboost_thread-mt
SRC=regen.cc regex.cc lexer.cc expr.cc exprutil.cc nfa.cc dfa.cc sfa.cc generator.cc $(SRC_)
else
SRC=regen.cc regex.cc lexer.cc expr.cc exprutil.cc nfa.cc dfa.cc generator.cc $(SRC_)
endif

ifeq ($(shell uname),Darwin)
MAKE_SHARED_LIBRARY=-dynamiclib #-exported_symbols_list libregen.symbols.darwin
else
MAKE_SHARED_LIBRARY=-shared -Wl,-soname,libregen.so #,--version-script=libregen.symbols
endif

CC=g++
CFLAGS=-fPIC -O3 -Wall -g3 -fno-operator-names $(REGENFLAGS)
OBJS=$(SRC:.cc=.o)
BINFLAG=-L$(PWD) -lregen -Xlinker -rpath -Xlinker $(PWD)

all: libregen.so recon fullmatch state_num regengrep tests/test_all bench.o

libregen.so: $(OBJS)
	$(CC) $(OBJS) $(MAKE_SHARED_LIBRARY) -o libregen.so $(CFLAGS) $(LIBTHREAD)

recon: recon.cc libregen.so
	$(CC) recon.cc $(OBJS) -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

fullmatch: tests/fullmatch.cc libregen.so
	$(CC) tests/fullmatch.cc -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

state_num: tests/state_num.cc libregen.so
	$(CC) tests/state_num.cc -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

regengrep: tests/regengrep.cc libregen.so
	$(CC) tests/regengrep.cc -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

test: tests/test_all
	@tests/test_all

bench: bench.o
	@./bench.o

tests/test_all: tests/test_all.cc libregen.so
	$(CC) tests/test_all.cc tests/gtest-all.cc tests/gtest_main.cc -o tests/test_all -pthread $(CFLAGS)  $(BINFLAG) $(LIBTHREAD)

bench.o: tests/bench.cc libregen.so
	$(CC) tests/bench.cc -o bench.o $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

.cc.o:
	$(CC) -c $< $(CFLAGS)

clean:
	rm -rf $(OBJS) tests/test_all bench.o sfa.h.gch recon test bench fullmatch state_num regengrep *.dSYM libregen.so

depend:
	sed '/^# DO NOT DELETE THIS LINE/q' Makefile > tmp
	g++ -MM $(CFLAGS) $(SRC) >> tmp
	mv tmp Makefile

# DO NOT DELETE THIS LINE -- make depend depends on it.
regen.o: regen.cc regen.h regex.h util.h lexer.h expr.h exprutil.h \
  generator.h dfa.h nfa.h jitter.h ext/xbyak/xbyak.h ext/str_util.hpp \
  sfa.h
regex.o: regex.cc regex.h regen.h util.h lexer.h expr.h exprutil.h \
  generator.h dfa.h nfa.h jitter.h ext/xbyak/xbyak.h ext/str_util.hpp \
  sfa.h
lexer.o: lexer.cc lexer.h util.h regen.h
expr.o: expr.cc expr.h util.h
exprutil.o: exprutil.cc exprutil.h expr.h util.h
nfa.o: nfa.cc nfa.h util.h
dfa.o: dfa.cc dfa.h regen.h util.h nfa.h expr.h jitter.h \
  ext/xbyak/xbyak.h ext/str_util.hpp
sfa.o: sfa.cc sfa.h regen.h regex.h util.h lexer.h expr.h exprutil.h \
  generator.h dfa.h nfa.h jitter.h ext/xbyak/xbyak.h ext/str_util.hpp
generator.o: generator.cc generator.h regex.h regen.h util.h lexer.h \
  expr.h exprutil.h nfa.h dfa.h jitter.h ext/xbyak/xbyak.h \
  ext/str_util.hpp sfa.h
jitter.o: jitter.cc jitter.h dfa.h regen.h util.h nfa.h expr.h \
  ext/xbyak/xbyak.h ext/str_util.hpp
