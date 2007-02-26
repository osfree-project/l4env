# directories we need to know
PKGDIR       ?= ../..
L4DIR        ?= $(PKGDIR)/../../..

SYSTEMS       = x86-l4v2

SRC_C_vc_test         = main.c
SRC_C_vc_plain        = vc_plain.c

DEFAULT_RELOC = 0x00200000
TARGET        = vc_test vc_plain

LIBS          += -lparsecmdline

MODE          = l4env_base

include $(L4DIR)/mk/prog.mk
