PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libio.a libio-ll.a
SYSTEMS		= x86-l4v2 amd64-l4v2

SRC_C		= init.c pci.c req_rel.c
SRC_S_libio.a	= io_info.S
SRC_S_libio-ll.a= io_info_ll.S
CLIENTIDL	= $(PKGNAME).idl
PRIVATE_INCDIR	= $(SRC_DIR)/../include .

include $(L4DIR)/mk/lib.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif

