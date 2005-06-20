proj = ../..
srcdir = $(proj)/src
build = $(proj)/build
parser = $(srcdir)/parser
core = $(srcdir)/newt_core

DEBUG = # -g
INCS = -I$(srcdir) -I$(core)/incs
EXTLIBS = 


include $(proj)/platform/$(shell uname)

NEWTLIBNAME	= NewtObjC
NEWTEXLIB	= $(build)/$(NEWTLIBNAME).$(NEWTLIBSUFFIX)
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

clean:
	rm -rf $(NEWTEXLIB) $(LIBOBJ) $(RUNTIMEXLIB)
	cd objc-runtime-x && make clean
