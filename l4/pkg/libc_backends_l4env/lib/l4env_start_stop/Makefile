PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0

# the name of your library
TARGET_x86-l4v2	= libdiet_be_l4env_start_stop.a
TARGET_x86-l4x0	= $(TARGET_x86-l4v2)

# list your .c files here
SRC_C	= l4libc_startup.c _exit.c abort.c

PRIVATE_INCDIR	= ../../../include ../../../dietlibc/include
#/home/mp26/erwin/drops/l4/pkg/l4xvfs_server/lib/dietlibc/include
#MODE		= host
LIBCINCDIR	= -I$(GCCINCDIR)
CPPFLAGS	= -nostdinc

DEBUG		= 1

include $(L4DIR)/mk/lib.mk
$(warning MODE=$(MODE))