PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = dope_presenter presenter_fprov
SYSTEMS          = x86-l4v2 x86-l4x0
DEFAULT_RELOC    = 0x00c00000
LIBS             = -ll4dope -lvscreen
PRIVATE_INCDIR   = $(PKGDIR)/server/include

SRC_C_dope_presenter  = presenter.c

SRC_C_presenter_fprov = presenter_fprov.c
LIBS_presenter_fprov  = -lgeneric_fprov -lparsecmdline

include $(L4DIR)/mk/prog.mk
