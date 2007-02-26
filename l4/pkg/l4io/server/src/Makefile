PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = l4io
MODE		= l4env
DEFAULT_RELOC   = 0x00b80000
SYSTEMS         = x86-l4v2 x86-l4x0

SRC_C           = main.c res.c pci.c jiffies.c
PRIVATE_INCDIR  = $(PKGDIR)/server/include \
                  $(PKGDIR)/server/lib-pci/include \
                  $(PKGDIR)/server/lib-omega0/include

PRIVATE_LIBDIR  = $(PKGDIR)/server/lib-omega0/src/OBJ-$(SYSTEM) \
                  $(PKGDIR)/server/lib-pci/src/OBJ-$(SYSTEM)

LIBS            = -lio-pci24 -lio-omega0 -lgeneric_io-server 
CFLAGS          = -fno-strict-aliasing

include $(L4DIR)/mk/prog.mk
