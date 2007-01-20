proj = ../..
srcdir = $(proj)/src
build = $(proj)/build
parser = $(srcdir)/parser
core = $(srcdir)/newt_core

prefix = /opt/local
exec_prefix = ${prefix}
sitedir = ${prefix}/lib/newt0
libffidir = ${prefix}

CC = gcc
AR = ar
RANLIB = ranlib
YACC = bison -y -d
LEX = flex
DEFS = -DHAVE_CONFIG_H
LIBS = 
DLEXT = dylib
EXEEXT = 
LDIMPORT = 
DEBUG = -g
LDFLAGS = $(DEBUG) -O2  
LDSHARED = $(CC) -dynamic -bundle -undefined suppress -flat_namespace

INCS = -I$(srcdir) -I$(core)/incs -I$(libffidir)/include
EXTLIBS = 

LIBFFI = $(libffidir)/lib/libffi.a

NEWTLIBNAME	= NativeCalls
NEWTEXLIB	= $(build)/$(NEWTLIBNAME).$(DLEXT)

LIBOBJ		= $(build)/$(NEWTLIBNAME).o
CFLAGS		+= $(DEBUG) -Wall -DDYLIBSUFFIX='"$(DLEXT)"'

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


all: $(build) $(NEWTEXLIB)

$(build):
	mkdir -p $(build)

$(NEWTEXLIB): $(LIBOBJ)
	$(LDSHARED) $< $(LDIMPORT) $(LIBFFI) -o $@

$(LIBOBJ): $(NEWTLIBNAME).c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

install::
	install -d -m 755 $(DESTDIR)$(sitedir)
	install -m 644 $(NEWTEXLIB) $(DESTDIR)$(sitedir)

clean:
	rm -rf $(NEWTEXLIB)
	rm -rf $(LIBOBJ)
