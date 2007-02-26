PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = dopecmd
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxdope -lpthread -lm
SRC_C            = dopecmd.c startup.c
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/dopecmd/include \
                   $(PKGDIR)/include

PRIVATE_LIBDIR   = $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM) 

vpath % $(PKGDIR)/examples/dopecmd/common 

include $(L4DIR)/mk/prog.mk

dopecmd.o: simpleterm.i

simpleterm.i: simpleterm.dpe
	$(PKGDIR)/tool/gen_dopecode $^ > $@

