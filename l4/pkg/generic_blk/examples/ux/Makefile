# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c driver.c command.c notification.c request.c

# target
TARGET =	blk-ux
MODE =		l4env
LIBS =		-lgeneric_blk-server -lclxfux.o
LIBCINCDIR = 	-I../../oskit
DEFAULT_RELOC = 0x00500000
SYSTEMS =       x86-l4v2 x86-l4x0

vpath %.c ../../oskit

# include prog role
include $(L4DIR)/mk/prog.mk

