PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2

DEFAULT_RELOC  = 0x00a00000

-include $(PKGDIR_OBJ)/Makeconf

TARGET         = dde26_bug_timersleep

SRC_C			= main.c

LIBS			+= -ldde_linux26.o -lddekit -lio -lomega0 -llist_alloc

PRIVATE_INCDIR 	= $(PKGDIR_ABS)/linux26/include $(MY_DDE_INCDIR) $(MY_LINUX26_INCDIR)  \
				  $(OBJ_BASE)/include/uclibc

LIBCINCDIR  = -nostdinc $(I_GCCINCDIR)
DEFINES     = -D__KERNEL__ -DDDE_LINUX $(KBUILD_DEFINES)
CPPFLAGS 	+= $(KBUILD_CPPFLAGS)

include $(PKGDIR)/linux26/Makeconf

include $(L4DIR)/mk/prog.mk
