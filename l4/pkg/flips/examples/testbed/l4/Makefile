PKGDIR      ?= ../../..
L4DIR       ?= $(PKGDIR)/../..

MODE         = l4env_dietlibc_minimal
TARGET       = testbed-client testbed-server
DEFAULT_RELOC_testbed-client = 0x01a00000
DEFAULT_RELOC_testbed-server = 0x01a08000
SRC_C_testbed-client = client.c
SRC_C_testbed-server = server.c
LIBS         = -lflips
SYSTEMS      = x86-l4v2

PRIVATE_INCDIR = $(PKGDIR)/examples/testbed/l4

vpath % $(PKGDIR)/examples/testbed/common

include $(L4DIR)/mk/prog.mk

