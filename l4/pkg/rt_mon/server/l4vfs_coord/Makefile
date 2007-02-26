# directories we need to know
PKGDIR       ?= ../..
L4DIR        ?=  $(PKGDIR)/../..

SYSTEMS       = x86-l4v2

# source files
SERVERIDL     = l4vfs_rt_mon.idl rt_mon_reg.idl

SRC_C         = coord.c clients.c helper.c

MODE          = l4env_minimal
DEFAULT_RELOC = 0x002fd000

LIBS         += -ll4vfs_name_server -ll4vfs_name_space_provider  \
                -ll4vfs_object_server-server                     \
                -ll4vfs_tree_helper

TARGET        = rt_mon_l4vfs_coord

include $(L4DIR)/mk/prog.mk
