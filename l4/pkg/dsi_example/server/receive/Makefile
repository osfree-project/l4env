# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C = 	receive.c
SERVERIDL =	receive.idl
PRIVATE_INCDIR =$(SRC_DIR)/../include

# target
TARGET =	dsiexmpl_receive
SYSTEMS =	x86-l4v2
MODE =		l4env
DEFAULT_RELOC = 0x00500000
LIBS =		-ldsi $(GCCLIB)

# include prog role
include $(L4DIR)/mk/prog.mk
