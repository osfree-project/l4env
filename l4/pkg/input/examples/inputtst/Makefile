PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS := x86-l4v2 amd64-l4v2
TARGET = inputtst 
TARGET_x86-l4v2 := inputtst-ux
DEFAULT_RELOC = 0x00900000
LIBS_inputtst = -linput -lomega0
LIBS_inputtst-ux = -linput-ux -lomega0

SRC_C = main.c

include $(L4DIR)/mk/prog.mk
