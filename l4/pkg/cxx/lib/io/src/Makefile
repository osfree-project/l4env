PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= iob_write.cc
SYSTEMS		= x86 arm amd64
TARGET		= libcxx_io

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

