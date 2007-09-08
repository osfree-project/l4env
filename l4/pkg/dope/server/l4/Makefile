SYSTEMS         := x86-l4v2 amd64-l4v2 arm-l4v2
PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

TARGET           = l4dope

DEFAULT_RELOC_x86   = 0x01b00000
DEFAULT_RELOC_amd64 = 0x01b00000
DEFAULT_RELOC_arm   = 0x00320000

PRIVATE_LIBDIR   = $(PKGDIR_OBJ)/server/common/OBJ-$(SYSTEM) \
                   $(PKGDIR_OBJ)/server/gfx/OBJ-$(SYSTEM)

LIBS_x86         = -lomega0
LIBS_amd64       = $(LIBS_x86) -luc_c++
LIBS_arm         = -larm_lcd.o -larm_drv_common.o -luc_c++
LIBS             = -ldm_phys -linput -lio $(LIBS_$(ARCH)) -ll4env -levents \
                   -lcommon -lgfx $(GCCLIB)

PRIVATE_INCDIR   = $(PKGDIR_ABS)/server/include \
                   $(DICE_INCDIR)

ifeq ($(ARCH),arm)
COL_MODE         = multi
endif
ifeq ($(ARCH),amd64)
COL_MODE         = multi
endif

CFLAGS_donsched-client.c = -ffunction-sections

SRC_C            = messenger.c vscr_server.c  donsched.c  \
                   server.c    startup.c      input.c     \
                   thread.c    timer.c        sharedmem.c

ifeq ($(COL_MODE),multi)
SRC_CC          += scrdrv_gen.cc
else
SRC_C           += scrdrv.c
endif

CXXFLAGS        += -fno-rtti -fno-exceptions

vpath messenger.c $(PKGDIR_ABS)/server/common

CLIENTIDL        = dopeapp.idl donsched.idl
SERVERIDL        = dope.idl vscr.idl donsched.idl
IDL_PKGDIR       = $(PKGDIR_OBJ) $(PKGDIR_OBJ)/server

include $(L4DIR)/mk/prog.mk

ifdef USE_RT_MON
LIBS_ALL        += -lrt_mon_client
CFLAGS          += -DUSE_RT_MON=1
endif
