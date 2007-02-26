PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libio.a
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= init.c pci.c req_rel.c
CLIENTIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/lib.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif
