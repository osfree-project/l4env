PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 arm-l4x0 amd64-l4v2 x86-l4secv2emu

TARGET   = libdiet_be_minimal_io.a

SRC_C    = write.c lseek.c

MODE     = l4env_dietlibc_minimal

include $(L4DIR)/mk/lib.mk
