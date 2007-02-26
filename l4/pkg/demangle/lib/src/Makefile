PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libdemangle.a
SYSTEMS		= x86 arm

MODE		= sigma0
SRC_C		= cplus-dem.c cp-demangle.c safe-ctype.c \
		  xmalloc.c xstrdup.c xexit.c memcmp.c
DEFINES		= -DHAVE_STDLIB_H -DHAVE_STRING_H

include $(L4DIR)/mk/lib.mk

CFLAGS		+= -fomit-frame-pointer -Os
