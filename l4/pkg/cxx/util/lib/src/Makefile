PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
CXXFLAGS	+= -fno-rtti -fno-exceptions
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= ipc_error_str.cc
SYSTEMS		= x86 arm
TARGET		= libcxx_util.a

include $(L4DIR)/mk/lib.mk

