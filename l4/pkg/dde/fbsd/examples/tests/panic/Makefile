PKGDIR     ?= ../../../..
L4DIR      ?= $(PKGDIR)/../..

TARGET      = t_panic
SYSTEMS     = x86-l4v2
DEFAULT_RELOC  = 0x00400000

SRC_C       = main.c


DDE_FBSD_ENV    = BSD
include $(PKGDIR)/fbsd/Makefile.inc
PRIVATE_INCDIR += $(DDE_FBSD_INCDIR)
DEFINES        += $(DDE_FBSD_DEFINES) -D_STDARG_H
ASFLAGS        += $(DDE_FBSD_ASFLAGS)
CFLAGS         += $(DDE_FBSD_CFLAGS)
LIBS           += $(DDE_FBSD_LIBS)


include $(L4DIR)/mk/prog.mk
