PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc -I$(GCCDIR)/include
SRC_C		= memset.c 
SRC_CC		= abort.cc
SYSTEMS		= x86-l4v2 arm-l4x0 
TARGET		= libsupc++-support

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

