# directories we need to know
PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../../..

SYSTEMS = x86-l4v2 x86-l4x0

SRC_C    = main.c

TARGET   = fprov_proxy
MODE     = l4env_base
DEFAULT_RELOC = 0x01620000
LIBS    += -lgeneric_fprov-server 

include $(L4DIR)/mk/prog.mk
