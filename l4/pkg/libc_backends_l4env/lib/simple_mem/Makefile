PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 arm-l4x0 amd64-l4v2
MODE    = l4env_minimal
TARGET  = libc_be_simple_mem.a
SRC_C   = mmap.c getpagesize.c
CFLAGS  = -ffunction-sections

include $(L4DIR)/mk/lib.mk
