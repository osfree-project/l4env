SYSTEMS = x86-l4v2
PKGDIR ?=  ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET = l4lx_flips

MODE = l4linux

SRC_C = server.c net_io.c notify.c events.c

SERVERIDL = flips.idl

LIBS = -levents -ll4vfs_connection -ll4vfs_network_server-server -ll4vfs_select \
       -ll4vfs_select_notify-server \
       -lloglinux -lpthread

include $(L4DIR)/mk/prog.mk
