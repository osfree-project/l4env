PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= sigma0
TARGET		= libnethub.a

SYSTEMS		= x86-l4v2

SRC_CC		= client.cc 
SRC_C           = cfg.c base.c

CLIENTIDL	= nh-client.idl nh-cfg.idl nh.idl
IDL_PKGDIR	= $(PKGDIR_OBJ)/base_lib

CXXFLAGS	+= -fno-rtti -fno-exceptions
include $(L4DIR)/mk/lib.mk
