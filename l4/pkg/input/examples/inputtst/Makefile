PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS           := x86-l4v2 amd64-l4v2 arm-l4x0
TARGET_x86-l4v2   := inputtst inputtst-ux
TARGET_amd64-l4v2 := inputtst inputtst-ux
TARGET_arm-l4x0   := inputtst-arm

DEFAULT_RELOC = 0x00900000

LIBS_inputtst     = -linput -lomega0
LIBS_inputtst-ux  = -linput-ux -lomega0
LIBS_inputtst-arm = -linput -larm_drv_common.o

SRC_C = main.c

include $(L4DIR)/mk/prog.mk
