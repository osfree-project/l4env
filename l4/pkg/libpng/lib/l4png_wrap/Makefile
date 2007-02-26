PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = libl4png_wrap.a
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C           = png_stream.c l4png_wrap.c 

CFLAGS		+= -I$(L4DIR)/include/l4/zlib

#DEBUG		= 1

include $(L4DIR)/mk/lib.mk
