# directories we need to know
PKGDIR         ?= ../..
L4DIR          ?= $(PKGDIR)/../..

SYSTEMS         = x86-l4v2

SRC_C           = main.c

DEFAULT_RELOC   = 0x0037e000
TARGET          = rt_mon_hist_demo

MODE            = l4env_minimal

LIBS           += -lrt_mon_client

include $(L4DIR)/mk/prog.mk
