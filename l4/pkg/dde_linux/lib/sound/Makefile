PKGDIR     ?= ../..
L4DIR      ?= $(PKGDIR)/../..

TARGET      = lib$(PKGNAME)-sound.a
SYSTEMS     = x86-l4v2

### specialties
SRC_C       = soundcore.c helpers.c dummy.c
PRIVATE_INCDIR = $(PKGDIR)/lib/include $(DDE_INCDIR) $(LINUX24_INCDIR)
LIBCINCDIR  = -nostdinc $(I_GCCINCDIR)

DEFINES     = -D__KERNEL__ -DDDE_LINUX
WARNINGS    = -Wall -Wstrict-prototypes -fno-strict-aliasing

include $(L4DIR)/mk/lib.mk
