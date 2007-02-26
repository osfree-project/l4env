PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
LIBCINCDIR	= -nostdinc
SRC_CC	       := uart_integrator.cc
SYSTEMS		= arm
TARGET		= libarm_uart.a

PRIVATE_INCDIR += $(SRC_DIR)/../include 

include $(L4DIR)/mk/lib.mk

CXXFLAGS += -fno-exceptions -fno-rtti

