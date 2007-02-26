PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libl4z.a
SYSTEMS		= x86 arm

SRC_C		= adler32.c compress.c crc32.c gzio.c \
		  uncompr.c deflate.c trees.c zutil.c inflate.c \
		  infback.c inftrees.c inffast.c

CFLAGS		+= -include ../../build/func_rename.h

DEBUG 		= 0

vpath %.h ../../dist
vpath %.c ../../dist

include $(L4DIR)/mk/lib.mk
