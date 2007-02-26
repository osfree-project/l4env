PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2

TARGET   = libc_be_io.o.a

SRC_C    = operations.c vector_io.c ioctl.c fcntl.c mmap_normal.c
CFLAGS   = -ffunction-sections
MODE     = l4env_minimal

include $(L4DIR)/mk/lib.mk
