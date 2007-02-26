PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = minitop
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxdope -lpthread -lm
SRC_C            = minitop.c startup.c getvalues.c thread.c
DPE_minitop.c    = minitop.dpe
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/minitop/include \
                   $(PKGDIR)/include

PRIVATE_LIBDIR   = $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM) 

vpath % $(PKGDIR)/examples/minitop/common 

include $(L4DIR)/mk/prog.mk
