PKGDIR      ?= ../../..
L4DIR       ?= $(PKGDIR)/../..

MODE         = l4env_dietlibc_minimal
TARGET       = idltest-client
DEFAULT_RELOC= 0x01a00000
SRC_C        = client.c
LIBS         = -lflips
SYSTEMS      = x86-l4v2

PRIVATE_INCDIR = $(PKGDIR)/include \
                 $(PKGDIR)/examples/idltest/include

include $(L4DIR)/mk/prog.mk

