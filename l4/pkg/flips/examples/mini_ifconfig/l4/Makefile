PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

MODE           = l4env_base
TARGET         = mini_ifconfig
DEFAULT_RELOC  = 0x01a80000
SRC_C          = main.c ifconfig.c

SYSTEMS        = x86-l4v2

LIBS          += -lc_be_socket_io -ll4vfs_net_io

PRIVATE_INCDIR = $(PKGDIR)/examples/mini_ifconfig/include
#                 $(L4DIR)/pkg/linux-24-headers/include

include $(L4DIR)/mk/prog.mk

