# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c driver.c command.c notification.c request.c

# do not build target anymore but files needed for blk-ux
#TARGET =	blk_oskit
MODE =		l4env
LIBS =		-lgeneric_blk-server
DEFAULT_RELOC = 0x00400000
SYSTEMS =       x86-l4v2

# include prog role
include $(L4DIR)/mk/prog.mk

