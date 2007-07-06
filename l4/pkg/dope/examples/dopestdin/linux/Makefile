PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = dopestdin
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxdope -lpthread -lm
SRC_C            = dopestdin.c
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/include

PRIVATE_LIBDIR   = $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM) 

include $(L4DIR)/mk/prog.mk

