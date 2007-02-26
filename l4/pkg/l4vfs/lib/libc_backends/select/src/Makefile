PKGDIR  ?= ../../../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 x86-l4x0

TARGET   = libc_be_select.a

SRC_C    = select.c select_listener.c select_internal-client.c \
           select_internal-server.c
CFLAGS   = -ffunction-sections
MODE     = l4env_minimal

PRIVATE_INCDIR = $(PKGDIR)/lib/libc_backends/select/idl/OBJ-$(ARCH)-$(L4API)

vpath % $(PKGDIR)/lib/libc_backends/select/idl/OBJ-$(ARCH)-$(L4API)

include $(L4DIR)/mk/lib.mk
