PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

# the name of your library
TARGET		= libdiet_be_minimal.a

# list your .c files here
SRC_C	= write.c _exit.c __main.c

PRIVATE_INCDIR	= ../../../include ../../../dietlibc/include
LIBCINCDIR	= -I$(GCCINCDIR)
CPPFLAGS	= -nostdinc
MODE		= libc

#DEBUG		= 1

include $(L4DIR)/mk/lib.mk
