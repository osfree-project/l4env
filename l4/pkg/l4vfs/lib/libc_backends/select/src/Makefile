PKGDIR  ?= ../../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2

TARGET   = libc_be_select.a

CLIENTIDL = select_internal.idl
SERVERIDL = select_internal.idl
IDL_PKGDIR = $(PKGDIR_OBJ)/lib/libc_backends/select
SRC_C    = select.c select_listener.c
CFLAGS   = -ffunction-sections
MODE     = l4env_minimal

include $(L4DIR)/mk/lib.mk
