PKGDIR ?= ../../..
L4DIR  ?= $(PKGDIR)/../..

MODE     = l4env_base
TARGET   = testbed2-client testbed2-server
DEFAULT_RELOC_testbed2-client = 0x01a00000
DEFAULT_RELOC_testbed2-server = 0x01a08000
SRC_C_testbed2-client = client.c
SRC_C_testbed2-server = server.c
LIBS    += -lc_be_socket_io -ll4vfs_net_io 
SYSTEMS  = x86-l4v2

vpath % $(PKGDIR)/examples/testbed2/common

include $(L4DIR)/mk/prog.mk
