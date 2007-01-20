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
LDFLAGS = $(DEBUG) -O2  
LDSHARED = $(CC) -dynamic -bundle -undefined suppress -flat_namespace

DEBUG = # -g
INCS = -I$(srcdir) -I$(core)/incs
EXTLIBS = 

NEWTLIBNAME	= NewtObjC
NEWTEXLIB	= $(build)/$(NEWTLIBNAME).$(DLEXT)
RUNTIMEXLIBNAME = objc-runtime-x.dylib
RUNTIMEXLIB = $(build)/objc-runtime-x.dylib

LIBOBJ		= \
	$(NEWTLIBNAME).o \
	AppKitFunctions.o \
	FoundationFunctions.o \
	ObjCObjects.o \
	TNewtObjCRef.o \
	Utils.o

CFLAGS		+= -Wall

%.o: %.m
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

all: $(build) $(RUNTIMEXLIB) $(NEWTEXLIB)

$(build):
	mkdir -p $(build)

$(RUNTIMEXLIB):
	cd objc-runtime-x && make
	cp objc-runtime-x/objc-runtime-x.dylib $(RUNTIMEXLIB)

$(NEWTEXLIB): $(LIBOBJ) $(RUNTIMEXLIB)
	$(LDSHARED) $(LIBOBJ) $(LDIMPORT) -framework Cocoa -o $@

install::
	install -d -m 755 $(DESTDIR)$(sitedir)
	install -m 644 $(NEWTEXLIB) $(RUNTIMEXLIB) $(DESTDIR)$(sitedir)

clean:
	rm -rf $(NEWTEXLIB) $(LIBOBJ) $(RUNTIMEXLIB)
	cd objc-runtime-x && make clean
