PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0

TARGET		= libc_be_socket_io.a

SRC_C		= socket.c
CFLAGS		= -ffunction-sections
MODE		= l4env_minimal

include $(L4DIR)/mk/lib.mk
