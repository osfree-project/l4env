PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 amd64-l4v2
TARGET	= omega0_ex_serial
SRC_C	= main.c serial.c

DEFAULT_RELOC = 0x00A00000
LIBS	= -lomega0
MODE	= sigma0

include $(L4DIR)/mk/prog.mk
