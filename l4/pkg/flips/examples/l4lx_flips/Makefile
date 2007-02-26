SYSTEMS        = x86-l4v2
PKGDIR        ?=  ../..
L4DIR         ?= $(PKGDIR)/../..

TARGET	       = l4lx_flips

MODE           = l4linux

SRC_C	       = server.c thread_table.c notify.c

SERVERIDL      = flips.idl

LIBS           = -lthread_linux -lsemaphore -lthread_linux -ll4env_err -lloglinux \
                 -ll4vfs_network_server-server -ll4vfs_select -ll4vfs_select_notify-server \
                 -lflips_l4vfs_server

include $(L4DIR)/mk/prog.mk
