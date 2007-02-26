PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0

# the name of your library
TARGET_x86-l4v2	= libdiet_be_l4xvfs.a
TARGET_x86-l4x0	= $(TARGET_x86-l4v2)

# list your .c files here
SRC_C	= l4xvfs_client_lib.c

PRIVATE_INCDIR	= ../../../include ../../../dietlibc/include
#/home/mp26/erwin/drops/l4/pkg/l4xvfs_server/lib/dietlibc/include
#MODE		= host
LIBCINCDIR	=
CPPFLAGS	= -nostdinc

DEBUG		= 1

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL = l4xvfs.idl

include $(L4DIR)/mk/lib.mk
