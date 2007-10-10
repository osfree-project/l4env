PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 amd64-l4v2 arm-l4v2

TARGET		= libc_be_socket_io.a

SRC_C		= socket.c
CFLAGS		= -ffunction-sections
MODE		= l4env_minimal

#DEBUG 		= 1

include $(L4DIR)/mk/lib.mk
