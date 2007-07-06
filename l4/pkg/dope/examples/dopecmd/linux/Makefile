PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = dopecmd
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxdope -lpthread -lm -lvscreen
SRC_C            = dopecmd.c startup.c
DPE_dopecmd.c    = simpleterm.dpe
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/dopecmd/include \
                   $(PKGDIR)/include

PRIVATE_LIBDIR   = $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM) 

vpath % $(PKGDIR)/examples/dopecmd/common

include $(L4DIR)/mk/prog.mk
