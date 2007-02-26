PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = textedit
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxdope -lvscreen -lpthread
SRC_C            = textedit.c
DPE_textedit.c   = textedit.dpe
PRIVATE_INCDIR   = . \
                   $(L4DIR)/pkg/dope/include \
                   $(PKGDIR)/server/include

PRIVATE_LIBDIR   = $(L4DIR)/pkg/dope/lib/dope/linux/OBJ-$(SYSTEM) 

vpath % $(PKGDIR)/examples/textedit/common

include $(L4DIR)/mk/prog.mk
