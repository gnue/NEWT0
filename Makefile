srcdir = ./src
build = ./build
ext = ./ext
parser = $(srcdir)/parser
core = $(srcdir)/newt_core
objdir = $(build)/obj
yytmp = $(objdir)/yytmp
tardir = $(shell basename `pwd`)_$(shell uname)
docdir = $(build)/html


VPATH = $(core)
DEBUG = # -g
INCS = -I$(srcdir) -I$(core)/incs -I$(srcdir)/parser -I$(yytmp)
EXTLIBS = 
STRIP = strip -x

include platform/$(shell uname)


ifndef NEWT
	NEWT         = $(build)/newt
endif


MAINOBJ       = $(objdir)/main.o

COREOBJS      = $(objdir)/NewtBC.o \
                $(objdir)/NewtEnv.o \
                $(objdir)/NewtFile.o \
                $(objdir)/NewtFns.o \
                $(objdir)/NewtGC.o \
                $(objdir)/NewtIO.o \
                $(objdir)/NewtMem.o \
                $(objdir)/NewtNSOF.o \
                $(objdir)/NewtObj.o \
                $(objdir)/NewtParser.o \
                $(objdir)/NewtPrint.o \
                $(objdir)/NewtStr.o \
                $(objdir)/NewtVM.o

PARSEROBJS    = $(yytmp)/y.tab.o \
                $(yytmp)/lex.yy.o \
                $(objdir)/lookup_words.o

NEWTLIBS	  = $(ext)/protoFILE \
				$(ext)/protoREGEX


######

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


### all

all: newt ext


### make directory

$(build):
	mkdir -p $@

$(objdir):
	mkdir -p $@

$(yytmp):
	mkdir -p $@

### newt

newt: $(build) $(objdir) $(yytmp) $(NEWT) $(LDIMPORT)

$(NEWT): $(MAINOBJ) $(PARSEROBJS) $(COREOBJS)
	$(CC) $(LDFLAGS) $(LIBS) $(MAINOBJ) $(PARSEROBJS) $(COREOBJS) $(LIBS) -o $@



### MAIN

$(MAINOBJ): $(srcdir)/main.c $(srcdir)/version.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


### PARSER

$(yytmp)/y.tab.c $(yytmp)/y.tab.h: $(parser)/newt.y
	$(YACC) -o $@ $<

$(yytmp)/lex.yy.c: $(parser)/newt.l $(yytmp)/y.tab.h
	$(LEX) -o$@ $<

$(objdir)/lookup_words.o: $(parser)/lookup_words.c $(parser)/lookup_words.h
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


### CORE

$(COREOBJS)::
	$(CC) $(CFLAGS) $(INCS) -c $(core)/$(notdir $*.c) -o $@


### ext

ext: $(NEWTLIBS)

$(NEWTLIBS)::
	$(MAKE) -C $@


### strip (for win)

strip:
	$(STRIP) $(NEWT) $(build)/*.$(NEWTLIBSUFFIX)


### ARCHIVE

copy:
	rm -rf $(build)/$(tardir)
	mkdir -p $(build)/$(tardir)
	cp $(NEWT) $(build)/$(tardir)
	cp $(build)/*.$(NEWTLIBSUFFIX) $(build)/$(tardir)
	cp -Rp COPYING README.* documents sample $(build)/$(tardir)


tgz: copy
	tar czf $(build)/$(tardir).tgz -C $(build) $(tardir)


### DOCUMENT GENERATE

doc:
	rm -rf $(docdir)
	mkdir -p $(docdir)
	cd misc; doxygen doxygen.conf


### TEST

test:
	$(NEWT) -C sample test_all.newt


### CLEAN

clean:
	rm -rf build/*
