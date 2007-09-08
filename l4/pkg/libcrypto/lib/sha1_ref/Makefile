PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= $(PKGNAME)_sha1_ref.a
BUILD_PIC = $(TARGET)
SYSTEMS = x86

# list your .c files here
SRC_C	= sha1.c glue.c

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL =

PRIVATE_INCDIR	= $(PKGDIR)/include

include $(L4DIR)/mk/lib.mk
