PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0

# the name of your library
TARGET_x86-l4v2	= libdiet_be_io.a
TARGET_x86-l4x0	= $(TARGET_x86-l4v2)

# list your .c files here
SRC_C	= file-table.c operations.c volumes.c

PRIVATE_INCDIR	= ../../../include ../../../dietlibc/include
LIBCINCDIR	= -I$(GCCINCDIR)
CPPFLAGS	= -nostdinc

#DEBUG		= 1

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
#CLIENTIDL = l4xvfs.idl

include $(L4DIR)/mk/lib.mk
