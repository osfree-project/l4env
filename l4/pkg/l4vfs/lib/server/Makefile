# directories we need to know
PKGDIR ?= ../..
L4DIR  ?=  $(PKGDIR)/../..

SYSTEMS = x86-l4v2

# source files
SERVERIDL_libl4vfs_mmap_object_server-server.a = mmap_object_server.idl       \
                                                 mmap.idl object_server.idl   \
                                                 common_io.idl basic_io.idl   \
                                                 basic_name_server.idl        \
                                                 container_io.idl

SERVERIDL_libl4vfs_object_server-server.a      = object_server.idl            \
                                                 common_io.idl basic_io.idl   \
                                                 basic_name_server.idl        \
                                                 container_io.idl

SERVERIDL_libl4vfs_name_server-server.a        = name_server.idl              \
                                                 extendable.idl common_io.idl \
                                                 basic_io.idl                 \
                                                 basic_name_server.idl        \
                                                 name_space_provider.idl      \
                                                 container_io.idl

SERVERIDL_libl4vfs_network_server-server.a     = network_server.idl           \
                                                 common_io.idl net_io.idl     \
                                                 connection.idl

SERVERIDL_libl4vfs_select_listener-server.a    = select_listener.idl

SERVERIDL_libl4vfs_select_notify-server.a      = select_notify.idl

SERVERIDL_libl4vfs_common_io_notify-server.a   = common_io_notify.idl


SERVERIDL_libl4vfs_mmap_object_server-server.p.a = $(SERVERIDL_libl4vfs_mmap_object_server-server.a)
SERVERIDL_libl4vfs_object_server-server.p.a      = $(SERVERIDL_libl4vfs_object_server-server.a)
SERVERIDL_libl4vfs_name_server-server.p.a        = $(SERVERIDL_libl4vfs_name_server-server.a)
SERVERIDL_libl4vfs_network_server-server.p.a     = $(SERVERIDL_libl4vfs_network_server-server.a)
SERVERIDL_libl4vfs_select_listener-server.p.a    = $(SERVERIDL_libl4vfs_select_listener-server.a)
SERVERIDL_libl4vfs_select_notify-server.p.a      = $(SERVERIDL_libl4vfs_select_notify-server.a)
SERVERIDL_libl4vfs_common_io_notify-server.p.a   = $(SERVERIDL_libl4vfs_common_io_notify-server.a)

SRC_C   = mkdir.c rev_resolve.c creat.c getdents.c ioctl.c fcntl.c unlink.c   \
          rmdir.c fsync.c lseek.c stat.c access.c

SRC_C_libl4vfs_common_io_notify-server.a   = common_io_notify.c
SRC_C_libl4vfs_network_server-server.a     = socketpair.c getpeername.c getsockopt.c
SRC_C_libl4vfs_common_io_notify-server.p.a = $(SRC_C_libl4vfs_common_io_notify-server.a)
SRC_C_libl4vfs_network_server-server.p.a   = $(SRC_C_libl4vfs_network_server-server.a)

MODE    = l4env_minimal

TARGET  = libl4vfs_mmap_object_server-server.a  \
          libl4vfs_object_server-server.a       \
          libl4vfs_name_server-server.a         \
          libl4vfs_network_server-server.a      \
          libl4vfs_select_listener-server.a     \
          libl4vfs_select_notify-server.a       \
          libl4vfs_common_io_notify-server.a

# include lib role
include $(L4DIR)/mk/lib.mk
