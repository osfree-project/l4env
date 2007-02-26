PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= iostream.cc
SYSTEMS		= x86 arm amd64
TARGET		= libcxx_io_kdebug

vpath %.ld $(PKGDIR)/lib/src/ARCH-$(ARCH)
vpath %.S $(PKGDIR)/lib/src/ARCH-$(ARCH)

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

