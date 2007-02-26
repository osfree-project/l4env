PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = libl4png.a
SYSTEMS         = x86

SRC_C           = pngerror.c pngget.c pngpread.c pngrio.c pngrutil.c \
		  pngvcrd.c pngwrite.c pngwutil.c png.c pnggccrd.c \
		  pngmem.c pngread.c pngrtran.c pngset.c pngtrans.c \
		  pngwio.c pngwtran.c ipow.c

CFLAGS          += -I$(L4DIR)/include/l4/zlib -include ../../build/func_rename.h

DEFINE		= PNG_USER_MEM_SUPPORTED

vpath %.h ../../dist
vpath %.c ../../dist

include $(L4DIR)/mk/lib.mk
