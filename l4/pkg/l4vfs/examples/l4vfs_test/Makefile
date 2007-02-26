# directories we need to know
PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2 x86-l4x0

SRC_C          = main.c

DEFAULT_RELOC  = 0x00980000
TARGET         = l4vfs_test

MODE           = l4env_base

include $(L4DIR)/mk/prog.mk
