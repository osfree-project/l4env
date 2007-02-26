PKGDIR      ?= ../../..
L4DIR       ?= $(PKGDIR)/../..

MODE         = l4env_dietlibc_minimal
TARGET       = idltest-clientserver
DEFAULT_RELOC= 0x01c00000
SRC_C        = client.c server.c main.c
DEFINES_client.c = -D'main=client_main' -D'l4libc_heapsize=client_heapsize' -D'LOG_tag=client_LOG_tag'
DEFINES_server.c = -D'main=server_main' -D'l4libc_heapsize=server_heapsize' -D'LOG_tag=server_LOG_tag'
LIBS         = -lflips
SYSTEMS      = x86-l4v2

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/examples/idltest/include

vpath client.c $(PKGDIR)/examples/idltest/client
vpath server.c $(PKGDIR)/examples/idltest/server

include $(L4DIR)/mk/prog.mk

