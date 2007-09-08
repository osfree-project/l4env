PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2

DEFAULT_RELOC  = 0x00a00000

-include $(PKGDIR_OBJ)/Makeconf

ifeq ($(CONFIG_DDE26_COMMON),y)
TARGET         = dde26_unit_test
endif

SRC_C          = main.c

LIBS          += -ldde_linux26 -lddekit -lio -llist_alloc -lparsecmdline -lcunit_dde

# DDE configuration
include $(PKGDIR)/linux26/Makeconf

include $(L4DIR)/mk/prog.mk
