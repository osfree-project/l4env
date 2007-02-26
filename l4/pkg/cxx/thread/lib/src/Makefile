PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

vpath %.cc $(PKGDIR)/thread/lib/src/ARCH-$(ARCH)

MODE		= libc
CXXFLAGS	+= -fno-rtti -fno-exceptions
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= thread.cc thread-arch.cc task.cc
SYSTEMS		= x86-l4x0 x86-l4v2 arm-l4x0
TARGET		= libcxx_thread.a

include $(L4DIR)/mk/lib.mk

