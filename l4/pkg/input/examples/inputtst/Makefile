PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET = inputtst inputtst-ux
DEFAULT_RELOC = 0x00900000
LIBS_inputtst = -linput -lomega0
LIBS_inputtst-ux = -linput-ux -lomega0

SRC_C = main.c

include $(L4DIR)/mk/prog.mk
