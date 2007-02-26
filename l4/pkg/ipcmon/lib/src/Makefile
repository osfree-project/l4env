PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= lib$(PKGNAME).a
SYSTEMS  = x86-l4v2

# list your .c files here
SRC_C	= libipcmon.c

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL = ipcmon.idl

include $(L4DIR)/mk/lib.mk
