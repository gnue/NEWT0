proj = ../..
srcdir = $(proj)/src
build = $(proj)/build
parser = $(srcdir)/parser
core = $(srcdir)/newt_core

LIBFFI_PREFIX = /opt/local

DEBUG = # -g
INCS = -I$(srcdir) -I$(core)/incs -I$(LIBFFI_PREFIX)/include
EXTLIBS = 


include $(proj)/platform/$(shell uname)

LIBFFI = $(LIBFFI_PREFIX)/lib/libffi.a

NEWTLIBNAME	= NativeCalls
NEWTEXLIB	= $(build)/$(NEWTLIBNAME).$(NEWTLIBSUFFIX)

LIBOBJ		= $(build)/$(NEWTLIBNAME).o
CFLAGS		+= -Wall -DDYLIBSUFFIX='"$(NEWTLIBSUFFIX)"'

.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


all: $(build) $(NEWTEXLIB)

$(build):
	mkdir -p $(build)

$(NEWTEXLIB): $(LIBOBJ)
	$(LDSHARED) $< $(LDIMPORT) $(LIBFFI) -o $@

$(LIBOBJ): $(NEWTLIBNAME).c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

clean:
	rm -rf $(NEWTEXLIB)
	rm -rf $(LIBOBJ)
