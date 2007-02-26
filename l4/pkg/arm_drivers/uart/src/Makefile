PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC         := uart-sa1100.cc uart-pxa.cc
SYSTEMS		= arm-l4x0
TARGET		= libarm_uart.a

include $(L4DIR)/mk/lib.mk

