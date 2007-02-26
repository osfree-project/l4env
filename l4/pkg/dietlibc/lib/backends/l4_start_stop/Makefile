PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

# the name of your library
TARGET	= libdiet_be_l4_start_stop.a

# list your .c files here
SRC_C	= _exit.c abort.c

#/home/mp26/erwin/drops/l4/pkg/l4xvfs_server/lib/dietlibc/include
MODE		= libc

PRIVATE_INCDIR  = ../../../include ../../../dietlibc/include

DEBUG		= 1

include $(L4DIR)/mk/lib.mk
