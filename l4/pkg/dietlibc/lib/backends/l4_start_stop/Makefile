PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu

TARGET   = libdiet_be_l4_start_stop.a

SRC_C    = _exit.c abort.c

MODE     = l4env_dietlibc_minimal

include $(L4DIR)/mk/lib.mk
