PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..
SYSTEMS       ?= x86-l4v2 x86-linux

TARGET         = libcommon.a
INSTALL_TARGET =
PRIVATE_INCDIR = $(PKGDIR_ABS)/server/include \
                 $(PKGDIR_ABS)/include
SRC_C          = appman.c     background.c button.c    scale.c       \
                 cache.c      clipping.c   container.c conv_fnt.c    \
                 scheduler.c  fontman.c    frame.c     grid.c        \
                 hashtab.c    keymap.c     label.c     variable.c    \
                 main.c       pool.c       viewman.c   redraw.c      \
                 tick.c       script.c     scrollbar.c relax.c       \
                 tokenizer.c  userstate.c  widman.c    window.c      \
                 winlayout.c  vscreen.c    gfx.c       loaddisplay.c \
                 gfx_scr16.c  gfx_img16.c  scope.c     gfx_img32.c   \
                 gfx_yuv420.c dopestd.c    screen.c    vtextscreen.c \
                 entry.c

#include $(L4DIR)/pkg/petze/mk/petze.mk
include $(L4DIR)/mk/lib.mk

