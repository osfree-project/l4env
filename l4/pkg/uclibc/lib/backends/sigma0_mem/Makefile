PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2

TARGET		= libuc_be_sigma0_mem.a
SRC_C		= mmap.c getpagesize.c
CFLAGS		= -ffunction-sections
MODE		= l4env_uclibc_minimal

include $(L4DIR)/mk/lib.mk
