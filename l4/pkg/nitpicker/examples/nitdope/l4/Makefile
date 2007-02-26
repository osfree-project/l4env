SYSTEMS        := x86-l4v2 amd64-l4v2
PKGDIR         ?= ../../..
L4DIR          ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

DOPEDIR        ?= $(L4DIR_ABS)/pkg/dopeng
DOPEOBJDIR     ?= $(OBJ_BASE)/pkg/dopeng

TARGET          = nitdope nitdope-ux
DEFAULT_RELOC   = 0x01b80000

PRIVATE_LIBDIR  = $(DOPEOBJDIR)/server/common/OBJ-$(SYSTEM) \
                  $(DOPEOBJDIR)/server/gfx/OBJ-$(SYSTEM)

LIBS_ALL        = -ldm_phys -lio -lomega0 -ll4env -levents $(GCCLIB)
LIBS_nitdope    = -linput $(LIBS_ALL)
LIBS_nitdope-ux = -linput-ux $(LIBS_ALL)

PRIVATE_INCDIR  = $(DOPEDIR)/server/include \
                  $(DOPEDIR)/include \
                  $(DICE_INCDIR)

DEFINES_donsched-client.c = -Ddonsched_recv_any=dummy_donsched_recv_any
CFLAGS_donsched-client.c  = -ffunction-sections

### generic sources ###

SRC_C  = appman.c     background.c button.c    scale.c       \
         cache.c      clipping.c   container.c conv_fnt.c    \
         scheduler.c  fontman.c    frame.c     grid.c        \
         hashtab.c    keymap.c     label.c     variable.c    \
         main.c       pool.c       relax.c     redraw.c      \
         tick.c       script.c     scrollbar.c scope.c       \
         tokenizer.c  userstate.c  widman.c    window.c      \
         winlayout.c  vscreen.c    gfx.c       loaddisplay.c \
         gfx_scr16.c  gfx_img16.c  gfx_img32.c gfx_yuv420.c  \
         dopestd.c    screen.c     vtextscreen.c entry.c

vpath % $(DOPEDIR)/server/common

### l4 specific sources ###

SRC_C += messenger.c    donsched.c  sharedmem.c \
         vscr_server.c  server.c    startup.c   \
         thread.c       timer.c

vpath % $(DOPEDIR)/server/l4
vpath messenger.c $(DOPEDIR)/server/common

### gfx sources ###

SRC_C += bigmouse.c   default_fnt.c lat0_12.c \
         smallmouse.c mono_fnt.c    title_fnt.c

vpath % $(DOPEDIR)/server/gfx

### idl stubs ###

CLIENTIDL   = donsched.idl dopeapp.idl
SERVERIDL   = donsched.idl dope.idl vscr.idl
IDL_PKGDIR  = $(DOPEOBJDIR)/server $(DOPEOBJDIR)

### nitpicker specific sources ###

SRC_C      += nitscrdrv.c nitinput.c nitviewman.c

CLIENTIDL  += nitpicker.idl
SERVERIDL  += nitevent.idl
IDL_PKGDIR += $(OBJ_BASE)/pkg/nitpicker

include $(L4DIR)/mk/prog.mk

