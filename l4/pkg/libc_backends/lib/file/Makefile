PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS		           = x86-l4v2 arm-l4v2 amd64-l4v2
MODE		           = tiny
TARGET		           = libc_be_file_ux.a libc_be_file_fprov.a
SRC_C_libc_be_file_ux.a	   = ux.c
SRC_C_libc_be_file_fprov.a = fprov.c
CFLAGS		           = -ffunction-sections

include $(L4DIR)/mk/lib.mk
