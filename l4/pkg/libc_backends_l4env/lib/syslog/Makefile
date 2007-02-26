PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 amd64-l4v2
MODE    = l4env_minimal
TARGET  = libc_be_syslog.a
SRC_C   = syslog.c
CFLAGS  = -ffunction-sections

include $(L4DIR)/mk/lib.mk
