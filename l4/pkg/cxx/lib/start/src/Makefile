PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= cxx_atexit.cc cxx_main.cc
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
TARGET		= libcxx_start

vpath %.ld $(PKGDIR)/lib/src/ARCH-$(ARCH)
vpath %.S $(PKGDIR)/lib/src/ARCH-$(ARCH)

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

