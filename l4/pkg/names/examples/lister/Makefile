SYSTEMS := x86-l4v2
PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SRC_C	= main.c
TARGET	= names_lister
MODE	= l4linux

PRIVATE_INCDIR = $(L4INCDIR)

LIBS	= -lnames $(ROOTLIB) -ll4sys

include $(L4DIR)/mk/prog.mk
