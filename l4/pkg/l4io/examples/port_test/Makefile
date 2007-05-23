PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = l4io_port_test
DEFAULT_RELOC   = 0x00900000
SYSTEMS         = x86-l4v2

LIBS            = -lio
SRC_CC          = main.cc

include $(L4DIR)/mk/prog.mk

CXXFLAGS       += -fno-rtti -fno-exceptions
