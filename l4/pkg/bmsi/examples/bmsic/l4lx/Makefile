PKGDIR	?= ../../..
L4DIR	?= $(PKGDIR)/../..

MODE	= l4linux
SYSTEMS	= x86-l4v2

TARGET	= bmsic

# list your .c files here
SRC_C	= main.c 

vpath %.c $(PKGDIR)/examples/bmsic/common

LIBS	= -lbmsi_builder -llogserver_capsule

include $(L4DIR)/mk/prog.mk
