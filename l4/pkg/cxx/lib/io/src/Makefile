PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= iob_write.cc
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2
TARGET		= libcxx_io

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

