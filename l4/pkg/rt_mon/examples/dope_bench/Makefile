# directories we need to know
PKGDIR         ?= ../..
L4DIR          ?= $(PKGDIR)/../..

SYSTEMS         = x86-l4v2

SRC_C           = main.c

DEFAULT_RELOC   = 0x0059e000
TARGET          = dope_bench

MODE            = l4env_minimal

PRIVATE_INCDIR  = .

LIBS           += -ll4dope -lvscreen

include $(L4DIR)/mk/prog.mk
