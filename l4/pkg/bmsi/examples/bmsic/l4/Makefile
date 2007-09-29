PKGDIR	?= ../../..
L4DIR	?= $(PKGDIR)/../..

MODE	= l4env
SYSTEMS	= x86-l4v2
DEFAULT_RELOC = 0x01900000

TARGET	= bmsic_l4

# list your .c files here
SRC_C	= main.c 

vpath %.c $(PKGDIR)/examples/bmsic/common

LIBS	= -lbmsi_builder 

include $(L4DIR)/mk/prog.mk
