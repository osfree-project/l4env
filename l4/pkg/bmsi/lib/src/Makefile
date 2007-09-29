PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= libbmsi_builder.a libbmsi_integrity.a

# list your .c files here
SRC_C_libbmsi_builder.a	= libb.c
SRC_C_libbmsi_integrity.a	= libu.c

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL = bmsi.idl


#LIBS	+= -llyonif

include $(L4DIR)/mk/lib.mk
