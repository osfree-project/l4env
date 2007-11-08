PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET = $(PKGNAME)
SRC_C = main.c dummies.c l4evdev.c
LIBS = -llinux.o -ldde_linux -lio -lomega0 -llist_alloc

DEFAULT_RELOC  = 0x01900000
SYSTEMS        = x86-l4v2

PRIVATE_INCDIR = $(PKGDIR)/server/lib-linux/include \
                 $(DDE_INCDIR) $(LINUX24_INCDIR)

PRIVATE_LIBDIR = $(PKGDIR_OBJ)/server/lib-linux/src/OBJ-$(SYSTEM)

DEFINES        = -D__KERNEL__ -DDDE_LINUX
WARNINGS       = -Wall -Wstrict-prototypes

include $(L4DIR)/mk/prog.mk
