NOTARGETSTOINSTALL = 1

PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = libgfx.a
SYSTEMS          = x86-l4v2 x86-linux amd64-l4v2 arm-l4v2
PRIVATE_INCDIR   = $(PKGDIR)/server/include
SRC_C            = bigmouse.c   default_fnt.c lat0_12.c \
                   smallmouse.c mono_fnt.c    title_fnt.c \
                   big_fnt.c

include $(L4DIR)/mk/lib.mk

