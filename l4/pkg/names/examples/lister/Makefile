SYSTEMS := x86-l4v2 x86-l4x0
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C	= main.c
TARGET	= names_lister
MODE	= host

PRIVATE_INCDIR	= $(PKGDIR)/server/include $(L4INCDIR)
LIBS		= -lnames -lrmgr

include $(L4DIR)/mk/prog.mk
