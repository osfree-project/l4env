PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= cxa_pure_virtual.cc cxa_pure_delete.cc
SRC_CC_arm-l4x0	= dso_handle.cc
SYSTEMS		= x86-l4x0 x86-l4v2 arm-l4x0 
TARGET		= libcxx_base

vpath %.ld $(PKGDIR)/lib/src/ARCH-$(ARCH)
vpath %.S $(PKGDIR)/lib/src/ARCH-$(ARCH)

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

