proj = ../..
srcdir = $(proj)/src
build = $(proj)/build
parser = $(srcdir)/parser
core = $(srcdir)/newt_core


DEBUG = # -g
INCS = -I$(srcdir) -I$(core)/incs
EXTLIBS = 


include $(proj)/platform/$(shell uname)


NEWTLIBNAME	= protoREGEX
NEWTEXLIB	= $(build)/$(NEWTLIBNAME).$(NEWTLIBSUFFIX)

LIBOBJ		= $(build)/$(NEWTLIBNAME).o


.c.o:
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@


all: $(build) $(NEWTEXLIB)

$(build):
	mkdir -p $(build)

$(NEWTEXLIB): $(LIBOBJ)
	$(LDSHARED) $< $(LDIMPORT) $(LIBREGEX) -o $@

$(LIBOBJ): $(NEWTLIBNAME).c
	$(CC) $(CFLAGS) $(INCS) -c $< -o $@

clean:
	rm -rf $(NEWTEXLIB)
	rm -rf $(LIBOBJ)
