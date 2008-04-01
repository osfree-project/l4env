PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 arm-l4v2 amd64-l4v2

TARGET  = libuc_be_simple_sleep.a
SRC_C   = sleep.c
CFLAGS  = -ffunction-sections
MODE    = l4env_minimal

include $(L4DIR)/mk/lib.mk
