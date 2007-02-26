PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 x86-l4x0 arm-l4x0
MODE    = l4env_minimal
TARGET  = libc_be_simple_mem.a
SRC_C   = mmap.c getpagesize.c
CFLAGS  = -ffunction-sections

include $(L4DIR)/mk/lib.mk
