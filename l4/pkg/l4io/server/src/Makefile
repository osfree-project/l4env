PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = l4io
MODE            = l4env
DEFAULT_RELOC   = 0x00c20000
SYSTEMS         = x86-l4v2 amd64-l4v2 arm-l4v2

SRC_C           = main.c res.c jiffies.c events.c ux.c
SRC_C_x86-l4v2  = pci.c
SRC_C_amd64-l4v2= pci.c
SRC_C_arm-l4v2  = nopci.c
PRIVATE_INCDIR  = $(PKGDIR)/server/include \
                  $(PKGDIR)/server/lib-pci/include \
                  $(PKGDIR)/server/lib-omega0/include

PRIVATE_LIBDIR  = $(PKGDIR_OBJ)/server/lib-omega0/src/OBJ-$(SYSTEM) \
                  $(PKGDIR_OBJ)/server/lib-pci/src/OBJ-$(SYSTEM)

LIBS_x86        = -lio-pci24 -lio-omega0
LIBS_amd64	= $(LIBS_x86)
LIBS            = $(LIBS_$(ARCH)) -levents \
                  -lgeneric_io-server -lio-idl
CFLAGS          = -fno-strict-aliasing

include $(L4DIR)/mk/prog.mk
