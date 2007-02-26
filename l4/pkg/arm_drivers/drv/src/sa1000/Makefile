PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SYSTEMS		= arm
SRC_C		= hw.c uart.c
TARGET		= libarm_drv-sa1000.a

include $(L4DIR)/mk/lib.mk

