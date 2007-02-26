# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =	 	filter.c
SERVERIDL =	filter.idl
PRIVATE_INCDIR =../../include

# target
TARGET =	dsiexmpl_filter
SYSTEMS =	x86-l4v2 x86-l4x0
MODE =		l4env
DEFAULT_RELOC =	0x00600000
LIBS =		-ldsi

# include prog role
include $(L4DIR)/mk/prog.mk
