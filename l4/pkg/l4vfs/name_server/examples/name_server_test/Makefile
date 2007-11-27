# directories we need to know
PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

SYSTEMS        = x86-l4v2 amd64-l4v2 arm-l4v2

SRC_C          = main.c

DEFAULT_RELOC  = 0x00980000
TARGET         = name_server_test

MODE           = l4env_minimal

LIBS           = -ll4vfs_name_server -ll4vfs_basic_name_server   \
                 -ll4vfs_name_space_provider -ll4vfs_basic_io    \
                 -ll4vfs_extendable -ll4vfs_common_io

include $(L4DIR)/mk/prog.mk
