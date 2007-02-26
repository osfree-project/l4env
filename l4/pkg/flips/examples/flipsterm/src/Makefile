SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
FTDIR            = $(PKGDIR)/examples/flipsterm

MODE             = l4env_base
TARGET           = flipsterm
DEFAULT_RELOC    = 0x01a00000
LIBS             = -ll4term -ll4dope -lc_be_socket_io -ll4vfs_net_io \
                   -lflips
SRC_C            = flipsterm.c
DEFINES         += -Dprintf=term_printf

PRIVATE_INCDIR   = . \
                   $(PKGDIR)/include \
                   $(FTDIR)/lib/include

OBJS            += $(FTDIR)/lib/cat/OBJ-$(SYSTEM)/libcat.o \
                   $(FTDIR)/lib/ifconfig/OBJ-$(SYSTEM)/libifconfig.o \
                   $(FTDIR)/lib/httpstart/OBJ-$(SYSTEM)/libhttpstart.o \
                   $(FTDIR)/lib/httpdump/OBJ-$(SYSTEM)/libhttpdump.o

include $(L4DIR)/mk/prog.mk
