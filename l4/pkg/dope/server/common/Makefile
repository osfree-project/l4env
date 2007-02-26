NOTARGETSTOINSTALL = 1

PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..
SYSTEMS         ?= x86-l4v2 x86-linux x86-l4x0

TARGET           = libcommon.a
PRIVATE_INCDIR   = $(PKGDIR)/server/include 
SRC_C            = appman.c    background.c    basegfx.c   button.c    \
                   cache.c     clipping.c      container.c conv_fnt.c  \
                   eventloop.c fontman.c       frame.c     grid.c      \
                   hashtab.c   img16data.c     img8idata.c keymap.c    \
                   main.c      pool.c          pslim.c     redraw.c    \
                   rtman.c     script.c        scrollbar.c terminal.c  \
                   tokenizer.c userstate.c     widman.c    window.c    \
                   winlayout.c winman.c        vscreen.c

include $(L4DIR)/mk/lib.mk

