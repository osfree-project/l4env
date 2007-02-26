PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..

MODE           = l4env_base
TARGET         = svtest
DEFAULT_RELOC  = 0x02000000
SRC_C          = utils.c       accept_test.c close_shutdown_test.c \
                 io_test_tcp.c listen_test.c socket_test.c \
                 bind_test.c   io_test.c     io_test_udp.c \
                 socket.c      misc.c        inet_addr.c   \
                 l4main.c
SYSTEMS        = x86-l4v2
WARNINGS       = -Wall -Wno-unused -Wno-implicit

DEFINES_socket.c              += -D'main=svtest_main'

LIBS          += -lc_be_socket_io -ll4vfs_net_io

PRIVATE_INCDIR = $(PKGDIR)/examples/svtest/include

vpath % $(PKGDIR)/examples/svtest/contrib

include $(L4DIR)/mk/prog.mk
