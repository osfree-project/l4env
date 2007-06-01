PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 arm-l4v2 amd64-l4v2
MODE     = l4env_minimal
TARGET   = libc_be_l4env_start_stop.a
SRC_C    = l4libc_startup.c _exit.c abort.c

include $(L4DIR)/mk/lib.mk
