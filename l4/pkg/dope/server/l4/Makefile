SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = l4dope l4dope-ux
DEFAULT_RELOC    = 0x01b00000

PRIVATE_LIBDIR   = $(PKGDIR)/server/common/OBJ-$(SYSTEM) \
                   $(PKGDIR)/server/gfx/OBJ-$(SYSTEM)

LIBS_ALL         = -ldm_phys -lio -lomega0 -ll4env -levents -lcommon -lgfx $(GCCLIB)
LIBS_l4dope      = -lio -linput $(LIBS_ALL)
LIBS_l4dope-ux   = -linput-ux $(LIBS_ALL)

PRIVATE_INCDIR   = $(PKGDIR)/server/include \
                   $(PKGDIR)/server/idl/OBJ-$(ARCH)-$(L4API) \
                   $(L4DIR)/tool/dice/include
DEFINES_donsched-client.c = -Ddonsched_recv_any=dummy_donsched_recv_any
CFLAGS_donsched-client.c = -ffunction-sections
SRC_C            = messenger.c vscr_server.c  donsched.c  \
                   scrdrv.c    server.c       startup.c   \
                   thread.c    timer.c        sharedmem.c \
                   input.c

vpath messenger.c       $(PKGDIR)/server/common
vpath donsched-client.c $(PKGDIR)/server/idl/OBJ-$(ARCH)-$(L4API)
vpath donsched-server.c $(PKGDIR)/server/idl/OBJ-$(ARCH)-$(L4API)

CLIENTIDL        = dopeapp.idl donsched.idl
SERVERIDL        = dope.idl vscr.idl donsched.idl

include $(L4DIR)/mk/prog.mk

ifdef USE_RT_MON
LIBS_ALL         += -lrt_mon_client
CFLAGS           += -DUSE_RT_MON=1
endif
