PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2 x86-l4x0

DEFAULT_RELOC  = 0x00980000
TARGET         = ipcmon_test2

LIBS	+= -lipcmon

SRC_C			= main.c

include $(L4DIR)/mk/prog.mk
