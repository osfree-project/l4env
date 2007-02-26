PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= iostream.cc
SYSTEMS		= x86 arm
TARGET		= libcxx_io_kdebug.a
CXXFLAGS	+= -fno-rtti

vpath %.ld $(PKGDIR)/lib/src/ARCH-$(ARCH)
vpath %.S $(PKGDIR)/lib/src/ARCH-$(ARCH)


include $(L4DIR)/mk/lib.mk

