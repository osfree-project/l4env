PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2 x86-l4secv2emu

DEPENDS_PKGS	= libsigma0

TARGET		= libdiet_be_sigma0_mem.a

SRC_C		= mmap.c getpagesize.c
CFLAGS		= -ffunction-sections

MODE		= l4env_dietlibc_minimal

include $(L4DIR)/mk/lib.mk
