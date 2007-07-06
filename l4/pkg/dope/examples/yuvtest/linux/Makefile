PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = yuvtest
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxdope -lpthread -lm -lvscreen
SRC_C            = yuvtest.c startup.c 
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/yuvtest/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/server/include
PRIVATE_LIBDIR   = $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM) \
                   $(PKGDIR)/lib/vscreen/linux/OBJ-$(SYSTEM)

vpath % $(PKGDIR)/examples/yuvtest/common 

include $(L4DIR)/mk/prog.mk
