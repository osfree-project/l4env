PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(USE_OSKIT),y)
TARGET		= test_ioport_access test_iopage_access test_clisti \
		  test_kernel_mem test_maphole
endif

MODE		= sigma0
DEFAULT_RELOC 	= 0x01000000
SRC_CC		= iotest.cc

SRC_CC_test_ioport_access = port_access.cc
SRC_CC_test_iopage_access = iopage_access.cc
SRC_CC_test_clisti        = clisti.cc
SRC_CC_test_kernel_mem    = kernel_mem.cc
SRC_CC_test_maphole       = maphole.cc

SRC_S		= int_entry.S

CXXFLAGS	= -fno-rtti -fno-exceptions

include $(L4DIR)/mk/prog.mk

