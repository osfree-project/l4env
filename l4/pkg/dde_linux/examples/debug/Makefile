PKGDIR     ?= ../..
L4DIR      ?= $(PKGDIR)/../..

TARGET      = dde_debug
DEFAULT_RELOC = 0x00800000
SYSTEMS     = x86-l4v2

SRC_C       = debug.c

### specialties
PRIVATE_INCDIR = $(DDE_INCDIR) $(LINUX24_INCDIR)
LIBCINCDIR  = -nostdinc -I$(GCCINCDIR)

LIBS        = -ldde_linux -lio -lomega0 -llist_alloc

DEFINES     = -D__KERNEL__ -DDDE_LINUX
WARNINGS    = -Wall -Wstrict-prototypes -fno-strict-aliasing

include $(L4DIR)/mk/prog.mk
