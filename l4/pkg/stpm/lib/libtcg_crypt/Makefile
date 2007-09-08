PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= libtcg_crypt.a
SYSTEMS = x86

# list your .c files here
SRC_C	= cryptoglue.c rsaglue.c random.c

PRIVATE_INCDIR = $(PKGDIR)/include

WARNINGS = -Wall -Wstrict-prototypes

include $(L4DIR)/mk/lib.mk
