PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
SRC_CC	       += uart_pl011.cc uart_sa1000.cc uart_pxa.cc
SYSTEMS		= arm
TARGET		= libarm_uart.a

PRIVATE_INCDIR += $(SRC_DIR)/../include 

include $(L4DIR)/mk/lib.mk

CXXFLAGS += -fno-exceptions -fno-rtti

