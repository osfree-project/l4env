PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
SRC_CC	       += uart_dummy.cc uart_pl011.cc uart_sa1000.cc uart_pxa.cc
SYSTEMS		= x86 amd64 arm
TARGET		= libarm_uart.a

PRIVATE_INCDIR += $(SRC_DIR)/../include 

include $(L4DIR)/mk/lib.mk
-include $(SRC_DIR)/Makeconf.$(ARM_PLATFORM_TYPE)

CXXFLAGS += -fno-exceptions -fno-rtti

