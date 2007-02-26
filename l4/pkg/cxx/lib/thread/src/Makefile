PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

vpath %.cc $(PKGDIR)/lib/thread/src/ARCH-$(ARCH)

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= thread.cc thread-arch.cc task.cc
SYSTEMS		= x86-l4v2 arm-l4x0 
TARGET		= libcxx_thread

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

