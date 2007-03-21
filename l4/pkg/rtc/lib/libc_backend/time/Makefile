PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 amd64-l4v2
TARGET   = libc_be_time.a
SRC_C    = time.c gettimeofday.c settimeofday.c l4libc_init_time.c
MODE     = l4env_minimal

include $(L4DIR)/mk/lib.mk
