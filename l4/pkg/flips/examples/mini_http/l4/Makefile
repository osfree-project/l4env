PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

MODE           = l4env_base
TARGET         = mini_http
DEFAULT_RELOC  = 0x01ad0000
SRC_C          = main.c
SYSTEMS        = x86-l4v2

LIBS         += -lc_be_socket_io -ll4vfs_net_io \
                -lc_be_select -ll4vfs_select -ll4vfs_select_listener-server

include $(L4DIR)/mk/prog.mk

