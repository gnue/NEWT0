srcdir = ./src
build = ./build
ext = ./ext
parser = $(srcdir)/parser
core = $(srcdir)/newt_core


DEBUG = # -g
INCS = -I$(srcdir) -I$(core)/incs -I$(srcdir)/parser -I$(build)
EXTLIBS = 
STRIP = strip -x

include platform/$(shell uname)


ifndef NEWT
	NEWT         = $(build)/newt
endif


MAINOBJ       = $(build)/main.o

COREOBJS      = $(build)/NewtBC.o \
                $(build)/NewtEnv.o \
                $(build)/NewtFile.o \
                $(build)/NewtFns.o \
                $(build)/NewtGC.o \
                $(build)/NewtIO.o \
                $(build)/NewtMem.o \
                $(build)/NewtObj.o \
                $(build)/NewtParser.o \
                $(build)/NewtPrint.o \
                $(build)/NewtStr.o \
                $(build)/NewtVM.o

PARSEROBJS    = $(build)/y.tab.o \
                $(build)/lex.yy.o \
                $(build)/lookup_words.o

NEWTLIBS	  = $(ext)/protoFILE \
				$(ext)/protoREGEX


######

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


### all

all: newt ext


### mkdir build

$(build):
	mkdir -p $(build)


### newt

newt: $(build) $(NEWT) $(LDIMPORT)

$(NEWT): $(MAINOBJ) $(PARSEROBJS) $(COREOBJS)
	$(CC) $(LDFLAGS) $(LIBS) $(MAINOBJ) $(PARSEROBJS) $(COREOBJS) $(LIBS) -o $@



### MAIN

$(MAINOBJ): $(srcdir)/main.c $(srcdir)/version.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


### PARSER

$(build)/y.tab.c $(build)/y.tab.h: $(parser)/newt.y
	$(YACC) -o $@ $<

$(build)/lex.yy.c: $(parser)/newt.l $(build)/y.tab.h
	$(LEX) -o$@ $<

$(build)/lookup_words.o: $(parser)/lookup_words.c $(parser)/lookup_words.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


### CORE

$(build)/NewtBC.o: $(core)/NewtBC.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtEnv.o: $(core)/NewtEnv.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtFile.o: $(core)/NewtFile.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtFns.o: $(core)/NewtFns.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtGC.o: $(core)/NewtGC.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtIO.o: $(core)/NewtIO.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtMem.o: $(core)/NewtMem.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtObj.o: $(core)/NewtObj.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtParser.o: $(core)/NewtParser.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtPrint.o: $(core)/NewtPrint.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtStr.o: $(core)/NewtStr.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

$(build)/NewtVM.o: $(core)/NewtVM.c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


### ext

ext: $(NEWTLIBS)

$(NEWTLIBS)::
	$(MAKE) -C $@


### strip

strip:
	$(STRIP) $(NEWT) $(build)/*.$(NEWTLIBSUFFIX)


### TEST

test:
	$(NEWT) -C sample test_all.newt


### CLEAN

clean:
	rm -rf build/*
