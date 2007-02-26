PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0

# the name of your library
TARGET_x86-l4v2	= libdiet_be_simple_mem.a
TARGET_x86-l4x0	= $(TARGET_x86-l4v2)

# list your .c files here
SRC_C	= mmap.c

PRIVATE_INCDIR	= ../../../include ../../../dietlibc/include
#/home/mp26/erwin/drops/l4/pkg/l4xvfs_server/lib/dietlibc/include
#MODE		= host
LIBCINCDIR	= -I$(GCCINCDIR)
CPPFLAGS	= -nostdinc

#DEBUG		= 1

include $(L4DIR)/mk/lib.mk
