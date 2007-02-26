PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2 x86-l4x0

# the name of your library
TARGET   = libl4vfs_basic_name_server.a    \
           libl4vfs_name_space_provider.a  \
           libl4vfs_extendable.a           \
           libl4vfs_common_io.a            \
           libl4vfs_basic_io.a             \
           libl4vfs_connection.a           \
           libl4vfs_net_io.a               \
           libl4vfs_select.a               \
           libl4vfs_common_io_notify.a

# list your .c files here
SRC_C_libl4vfs_basic_name_server.a   = basic_name_server.c
SRC_C_libl4vfs_name_space_provider.a = name_space_provider.c
SRC_C_libl4vfs_extendable.a          = extendable.c
SRC_C_libl4vfs_common_io.a           = common_io.c
SRC_C_libl4vfs_basic_io.a            = basic_io.c container_io.c \
                                       mmap_io.c
SRC_C_libl4vfs_connection.a          = connection.c
SRC_C_libl4vfs_net_io.a              = net_io.c
SRC_C_libl4vfs_select.a              = select_notify.c           \
                                       select_listener.c

MODE     = l4env_minimal
CLIENTIDL_libl4vfs_basic_name_server.a   = basic_name_server.idl
CLIENTIDL_libl4vfs_name_space_provider.a = name_space_provider.idl
CLIENTIDL_libl4vfs_extendable.a          = extendable.idl
CLIENTIDL_libl4vfs_common_io.a           = common_io.idl
CLIENTIDL_libl4vfs_basic_io.a            = basic_io.idl container_io.idl \
                                           mmap.idl
CLIENTIDL_libl4vfs_connection.a          = connection.idl
CLIENTIDL_libl4vfs_net_io.a              = net_io.idl
CLIENTIDL_libl4vfs_select.a              = select_notify.idl             \
                                           select_listener.idl
CLIENTIDL_libl4vfs_common_io_notify.a    = common_io_notify.idl

include $(L4DIR)/mk/lib.mk
