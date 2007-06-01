PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS           := x86-l4v2 amd64-l4v2 #arm-l4v2
FILES_x86-l4v2    := inputtst
FILES_amd64-l4v2  := inputtst
FILES_arm-l4v2    := inputtst-arm
TARGET             = $(FILES_$(OSYSTEM))

DEFAULT_RELOC = 0x00900000

LIBS_inputtst     = -linput -lio -lomega0
LIBS_inputtst-arm = -linput -larm_drv_common.o

SRC_C = main.c

include $(L4DIR)/mk/prog.mk
