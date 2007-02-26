SYSTEMS         := x86-l4v2
PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = l4dope l4dope-ux
DEFAULT_RELOC    = 0x01b00000

PRIVATE_LIBDIR   = $(PKGDIR_OBJ)/server/common/OBJ-$(SYSTEM) \
                   $(PKGDIR_OBJ)/server/gfx/OBJ-$(SYSTEM)

LIBS_ALL         = -ldm_phys -lio -lomega0 -ll4env -levents -lcommon -lgfx $(GCCLIB)
LIBS_l4dope      = -linput -lio $(LIBS_ALL)
LIBS_l4dope-ux   = -linput-ux $(LIBS_ALL)

PRIVATE_INCDIR   = $(PKGDIR_ABS)/server/include

DEFINES_donsched-client.c = -Ddonsched_recv_any=dummy_donsched_recv_any
CFLAGS_donsched-client.c = -ffunction-sections

SRC_C            = messenger.c vscr_server.c  donsched.c  \
                   scrdrv.c    server.c       startup.c   \
                   thread.c    timer.c        sharedmem.c \
                   input.c

vpath messenger.c $(SRC_DIR)/../common

CLIENTIDL        = dopeapp.idl donsched.idl
SERVERIDL        = dope.idl vscr.idl donsched.idl
IDL_PKGDIR       = $(PKGDIR_OBJ) $(PKGDIR_OBJ)/server

include $(L4DIR)/mk/prog.mk

ifdef USE_RT_MON
LIBS_ALL        += -lrt_mon_client
CFLAGS          += -DUSE_RT_MON=1
endif
