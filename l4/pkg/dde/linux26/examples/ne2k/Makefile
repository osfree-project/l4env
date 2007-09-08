PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2

DEFAULT_RELOC  = 0x00a00000

-include $(PKGDIR_OBJ)/Makeconf

ifeq ($(CONFIG_DDE26_NET),y)
TARGET         = ne2k_dde26
endif

SRC_C			= ne2k-pci.c 8390.c main.c arping.c

LIBS			+= -ldde_linux26_net -ldde_linux26 -lddekit -lio -lomega0 -llist_alloc

PRIVATE_INCDIR 	= $(PKGDIR_ABS)/linux26/include $(MY_DDE_INCDIR) $(MY_LINUX26_INCDIR)  \
				  $(OBJ_BASE)/include/uclibc

LIBCINCDIR  = -nostdinc -I$(GCCINCDIR)
DEFINES     = -D__KERNEL__ -DDDE_LINUX\
			  $(KBUILD_DEFINES)
CPPFLAGS 	+= $(KBUILD_CPPFLAGS)

include $(PKGDIR)/linux26/Makeconf

include $(L4DIR)/mk/prog.mk

foo :
	@echo $(L4INCDIR)
	@echo $(OBJ_BASE)
