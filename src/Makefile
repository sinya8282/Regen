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
MAKE_SHARED_LIBRARY=-dynamiclib  #-exported_symbols_list libregen.symbols.darwin
else
MAKE_SHARED_LIBRARY=-shared -Wl,-soname,libregen.so #,--version-script=libregen.symbols
endif

CC=g++
CFLAGS=-fPIC -O3 -Wall -fno-operator-names $(REGENFLAGS)
BINDIR=bin
OBJS=$(SRC:.cc=.o)
BINFLAG=-L$(PWD)/$(BINDIR) -lregen -Xlinker -rpath -Xlinker $(PWD)/$(BINDIR)
APP=$(addprefix $(BINDIR)/, recon fullmatch state_num regengrep bench)

lib: $(BINDIR)/libregen.so

app: $(APP)

all: lib $(BINDIR)/test_all app

$(BINDIR)/libregen.so: $(OBJS)
	$(CC) $(OBJS) $(MAKE_SHARED_LIBRARY) -o $(BINDIR)/libregen.so $(CFLAGS) $(LIBTHREAD)

$(BINDIR)/recon: recon.cc $(BINDIR)/libregen.so
	$(CC) recon.cc $(OBJS) -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

$(BINDIR)/fullmatch: app/fullmatch.cc $(BINDIR)/libregen.so
	$(CC) app/fullmatch.cc -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

$(BINDIR)/state_num: app/state_num.cc $(BINDIR)/libregen.so
	$(CC) app/state_num.cc -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

$(BINDIR)/regengrep: app/regengrep.cc $(BINDIR)/libregen.so
	$(CC) app/regengrep.cc -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

test: $(BINDIR)/test_all
	@$(BINDIR)/test_all

bench: $(BINDIR)/bench
	@$(BINDIR)/bench

$(BINDIR)/test_all: tests/test_all.cc $(BINDIR)/libregen.so
	$(CC) tests/test_all.cc tests/gtest-all.cc tests/gtest_main.cc -o $@ -pthread $(CFLAGS)  $(BINFLAG) $(LIBTHREAD)

$(BINDIR)/bench: tests/bench.cc $(BINDIR)/libregen.so
	$(CC) tests/bench.cc -o $@ $(CFLAGS) $(BINFLAG) $(LIBTHREAD)

.cc.o:
	$(CC) -c $< $(CFLAGS)

objclean:
	rm -rf *.o

clean: objclean
	rm -rf $(BINDIR)/*

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
