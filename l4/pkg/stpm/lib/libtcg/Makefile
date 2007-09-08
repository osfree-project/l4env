PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= libtcg.a
SYSTEMS = x86

# list your .c files here
SRC_C	= rand.c transmit.c buildbuff.c basic.c quote.c oiaposap.c hmac.c \
	  keys.c pcrs.c sha.c seal.c owner.c counter.c

VPATH  += $(PKGDIR)/contrib/libtcg

PRIVATE_INCDIR = $(PKGDIR)/include \
		 $(PKGDIR)/include/sha1 \
		 $(PKGDIR)/include/rsaref2

WARNINGS = -Wall -Wstrict-prototypes

include $(L4DIR)/mk/lib.mk
