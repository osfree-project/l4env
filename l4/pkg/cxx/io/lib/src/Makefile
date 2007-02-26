PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
CXXFLAGS	+= -fno-rtti -fno-exceptions
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC		= iob_write.cc
SYSTEMS		= x86-l4x0 x86-l4v2 arm-l4x0
TARGET		= libcxx_io.a


include $(L4DIR)/mk/lib.mk

