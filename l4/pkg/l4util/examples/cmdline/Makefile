# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

SRC_C =		main.c
MODE		= host

TARGET =	cmdlineex
PRIVATE_INCDIR	= $(L4INCDIR)
LIBS		= -lparsecmdline -lloglinux
SYSTEMS		= x86

include $(L4DIR)/mk/prog.mk

