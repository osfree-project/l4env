PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c driver.c command.c notification.c request.c

# target
TARGET =	blk-ux
LIBS =		-lgeneric_blk-server -lclxfux.o -ll4util
DEFAULT_RELOC = 0x00560000
SYSTEMS =       x86-l4v2

vpath %.c $(SRC_DIR)/../oskit

# include prog role
include $(L4DIR)/mk/prog.mk

LIBCINCDIR	+= -I$(SRC_DIR)/../oskit
