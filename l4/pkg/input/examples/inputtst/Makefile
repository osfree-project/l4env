PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS           := x86-l4v2 amd64-l4v2 arm-l4v2
TARGET             = inputtst

DEFAULT_RELOC = 0x00900000

LIBS_x86   = -linput -lio -lomega0
LIBS_amd64 = -linput -lio -lomega0
LIBS_arm   = -linput -lio
LIBS       = $(LIBS_$(ARCH))

SRC_C = main.c

include $(L4DIR)/mk/prog.mk
