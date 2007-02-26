PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= sigma0
TARGET		= libnethub.a

SYSTEMS		= x86-l4v2 x86-l4x0

SRC_CC		= client.cc 
SRC_C           = cfg.c base.c

CLIENTIDL	= nh-client.idl nh-cfg.idl nh.idl

CXXFLAGS	+= -fno-rtti -fno-exceptions
include $(L4DIR)/mk/lib.mk

vpath %.c $(PKGDIR)/base_lib/idl/OBJ-$(ARCH)-$(L4API)
