PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= ipc_error_str.cc alloc_list.cc
SYSTEMS		= x86 arm
TARGET		= libcxx_util

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

