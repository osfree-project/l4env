PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu
MODE		= tiny
TARGET		= libc_be_self_mem.a
SRC_C		= mmap.c getpagesize.c
CFLAGS		= -ffunction-sections

include $(L4DIR)/mk/lib.mk
