SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = termtest
DEFAULT_RELOC    = 0x00a80000
LIBS             = -ll4term -ll4dope 
SRC_C            = termtest.c startup.c
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/include \
                   $(PKGDIR)/examples/termtest/include

vpath % $(PKGDIR)/examples/termtest/common 

include $(L4DIR)/mk/prog.mk

