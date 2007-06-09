PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= lib$(PKGNAME).a

# list your .c files here
SRC_C	= buffer.c event.c nitpicker.c view.c

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL = nitpicker.idl
SERVERIDL = nitevent.idl

include $(L4DIR)/mk/lib.mk
