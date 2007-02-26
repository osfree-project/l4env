PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 x86-l4x0
DEFINES  = -DL4LIBC_HEAPSIZE=1024*1024
MODE     = l4env_minimal
TARGET   = libc_be_buddy_slab_mem.a
SRC_C_libc_be_buddy_slab_mem.a = buddy.c malloc.c

include $(L4DIR)/mk/lib.mk
