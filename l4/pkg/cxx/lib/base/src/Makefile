PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= cxa_pure_virtual.cc cxa_pure_delete.cc
SRC_CC_arm     	= dso_handle.cc
SYSTEMS		= x86 arm amd64
TARGET		= libcxx_base

include $(PKGDIR)/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk
