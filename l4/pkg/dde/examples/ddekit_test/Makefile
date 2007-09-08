PKGDIR       ?= ../..
L4DIR        ?= $(PKGDIR)/../..

SYSTEMS       = x86-l4v2

DEFAULT_RELOC = 0x00a10000
TARGET        = ddekit_test

SRC_C         = main.c

LIBS         += -lddekit -lio -ll4util -llist_alloc

#LIBCINCDIR    = -nostdinc -I$(GCCINCDIR)

WARNINGS      = -Wall -Wstrict-prototypes -fno-strict-aliasing

include $(L4DIR)/mk/prog.mk
