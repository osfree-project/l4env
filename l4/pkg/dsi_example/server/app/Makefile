# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c
PRIVATE_INCDIR = ../../include

# target
TARGET =	dsiexmpl_app
SYSTEMS =	x86-l4v2 x86-l4x0
MODE =		l4env
DEFAULT_RELOC =	0x00700000
LIBS =		-ldsiexmpl_send \
		-ldsiexmpl_receive \
		-ldsiexmpl_filter \
		-ldsi

# include prog role
include $(L4DIR)/mk/prog.mk
