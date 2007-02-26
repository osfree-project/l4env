PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 x86-l4x0

TARGET   = libc_be_file-table.a

SRC_C    = file-table.c volumes.c
CFLAGS   = -ffunction-sections
MODE     = l4env_minimal

include $(L4DIR)/mk/lib.mk
