PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= io_backend.cc
SYSTEMS		= x86-l4x0 x86-l4v2 arm-l4x0
TARGET		= libcxx_linux_io

include $(L4DIR)/pkg/cxx/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

