PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libl4z.a
SYSTEMS		= x86 arm amd64

SRC_C		= adler32.c compress.c crc32.c gzio.c \
		  uncompr.c deflate.c trees.c zutil.c inflate.c \
		  infback.c inftrees.c inffast.c

CFLAGS		+= -include $(SRC_DIR)/func_rename.h

DEBUG 		= 0

vpath %.h $(SRC_DIR)/../dist
vpath %.c $(SRC_DIR)/../dist

include $(L4DIR)/mk/lib.mk
