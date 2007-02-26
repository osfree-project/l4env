SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = l4dope l4dope-ux
DEFAULT_RELOC    = 0x01b00000

PRIVATE_LIBDIR   = $(PKGDIR)/server/common/OBJ-$(SYSTEM) \
                   $(PKGDIR)/server/gfx/OBJ-$(SYSTEM)

LIBS_ALL         = -ldm_phys -lio -lomega0 -ll4env -lcommon -lgfx $(GCCLIB)
LIBS_l4dope      = -linput $(LIBS_ALL)
LIBS_l4dope-ux   = -linput-ux $(LIBS_ALL)

PRIVATE_INCDIR   = $(PKGDIR)/server/include \
                   $(L4DIR)/tool/dice/include
SRC_C            = messenger.c pslim_server.c \
                   scrdrv.c server.c    startup.c      \
                   thread.c timer.c     sharedmem.c  \
                   input.c  vscr_server.c

vpath messenger.c $(PKGDIR)/server/common

CLIENTIDL        = dopeapp.idl
SERVERIDL        = dope.idl pslim.idl vscr.idl

#include $(L4DIR)/pkg/petze/mk/petze.mk
include $(L4DIR)/mk/prog.mk

