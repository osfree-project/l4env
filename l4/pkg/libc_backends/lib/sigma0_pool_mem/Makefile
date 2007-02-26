PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2 x86-l4secv2emu
MODE		= tiny
TARGET		= libc_be_sigma0_pool_mem.a
SRC_C		= mmap.c getpagesize.c
CFLAGS		= -ffunction-sections
DEPENDS_PKGS	= libsigma0

include $(L4DIR)/mk/lib.mk
