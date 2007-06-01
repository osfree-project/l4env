PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 arm-l4v2 amd64-l4v2
TARGET   = libuc_be_minimal_io.a
SRC_C    = write.c lseek.c close.c fcntl.c
MODE     = l4env_uclibc_minimal

include $(L4DIR)/mk/lib.mk
