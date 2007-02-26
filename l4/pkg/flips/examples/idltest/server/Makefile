PKGDIR      ?= ../../..
L4DIR       ?= $(PKGDIR)/../..

MODE         = l4env_dietlibc_minimal
TARGET       = idltest-server
DEFAULT_RELOC= 0x01a40000
SRC_C        = server.c
LIBS         = -lflips
SYSTEMS      = x86-l4v2

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/examples/idltest/include

include $(L4DIR)/mk/prog.mk

