# directories we need to know
PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../../..


SYSTEMS       = x86-l4v2 x86-l4x0

SRC_C         = main.c clients.c

TARGET        = l4vfs_log
MODE          = l4env_minimal
DEFAULT_RELOC = 0x0043e000
LIBS         += -ll4vfs_name_server -ll4vfs_name_space_provider  \
                -ll4vfs_object_server-server                     \
                -ll4vfs_tree_helper

include $(L4DIR)/mk/prog.mk
