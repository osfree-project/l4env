# directories we need to know
PKGDIR        ?= ..
L4DIR         ?= $(PKGDIR)/../../..

SYSTEMS        = x86-l4v2

SRC_C          = main.c

DEFAULT_RELOC  = 0x02000000
TARGET         = simple_file_server_test

MODE           = l4env_base

LIBS          += -lc_be_select -ll4vfs_select -ll4vfs_select_listener-server

include $(L4DIR)/mk/prog.mk
