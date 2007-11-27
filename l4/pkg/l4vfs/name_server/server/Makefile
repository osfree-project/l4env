# directories we need to know
PKGDIR       ?= ../..
L4DIR        ?= $(PKGDIR)/../..

SYSTEMS	      = x86-l4v2 arm-l4v2 amd64-l4v2

# source files
SRC_C         = main.c volumes.c dirs.c mount_table.c resolve.c pathnames.c \
                clientstate.c

TARGET        = name_server
MODE          = l4env_minimal
LIBS         += -ll4vfs_basic_name_server -ll4vfs_name_server-server

DEFAULT_RELOC = 0x0151e000

include $(L4DIR)/mk/prog.mk
