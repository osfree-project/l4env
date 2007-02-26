PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = termtest
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxterm -llinuxdope -lpthread -lm
SRC_C            = termtest.c startup.c

PRIVATE_INCDIR   = . \
                   $(PKGDIR)/include \
                   $(PKGDIR)/examples/termtest/include

PRIVATE_LIBDIR   = $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM) \
                   $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM)

vpath % $(PKGDIR)/examples/termtest/common 

include $(L4DIR)/mk/prog.mk

