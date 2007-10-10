PKGDIR             ?= ..
L4DIR              ?= $(PKGDIR)/../..

MODE                = l4env_base

SYSTEMS             = x86-l4v2 amd64-l4v2 arm-l4v2

IDL                 = basic_io.idl              \
                      basic_name_server.idl     \
                      connection.idl            \
                      container_io.idl          \
                      select_notify.idl         \
                      select_listener.idl       \
                      extendable.idl            \
                      mmap.idl                  \
                      mmap_object_server.idl    \
                      name_server.idl           \
                      name_space_provider.idl   \
                      net_io.idl                \
                      object_server.idl         \
                      network_server.idl        \
                      common_io.idl             \
                      common_io_notify.idl

# enable external servers
IDL_EXPORT_SKELETON = basic_io.idl              \
                      basic_name_server.idl     \
                      connection.idl            \
                      container_io.idl          \
                      extendable.idl            \
                      mmap.idl                  \
                      mmap_object_server.idl    \
                      name_server.idl           \
                      name_space_provider.idl   \
                      net_io.idl                \
                      object_server.idl         \
                      common_io.idl             \
                      common_io_notify.idl      \
                      network_server.idl        \
                      select_notify.idl         \
                      select_listener.idl

IDL_FLAGS           = -t -ffree-mem-after-reply # -fforce-corba-alloc -ftrace-dump-msgbuf -ftrace-dump-msgbuf-dwords=4 -ftrace-function=LOG

include $(L4DIR)/mk/idl.mk
