# Makefile for omega0 server
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS	= x86-l4v2 amd64-l4v2
TARGET	= omega0
SRC_C	= main.c irq_threads.c globals.c server.c pic.c create_threads.c \
	  events.c
MODE	= sigma0
LIBS	= -lparsecmdline -levents

DEFAULT_RELOC = 0x00360000
PRIVATE_INCDIR = $(PKGDIR)/server/include

include $(L4DIR)/mk/prog.mk
