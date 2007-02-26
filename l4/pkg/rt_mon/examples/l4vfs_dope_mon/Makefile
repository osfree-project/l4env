# directories we need to know
PKGDIR         ?= ../..
L4DIR          ?= $(PKGDIR)/../..

SYSTEMS         = x86-l4v2 x86-l4x0

PRIVATE_INCDIR  =+ $(PKGDIR)/examples/common

vpath %          $(PKGDIR)/examples/common

SRC_C           = main.c tree.c vis.c wins.c color.c

DEFAULT_RELOC   = 0x0053f000
TARGET          = rt_l4vfs_dope_mon

MODE            = l4env_base

LIBS           += -lrt_mon_l4vfs_mon -lrt_mon_client -ll4dope -lvscreen  \
                  -lvfb_drawing -lvfb_icons

include $(L4DIR)/mk/prog.mk
