PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = libio-pci24.a
SYSTEMS         = x86-l4v2 x86-l4x0
MODE		= l4env
INSTALL_TARGET  =
SRC_C           = glue.c

### specialties
PRIVATE_INCDIR  = $(PKGDIR)/server/lib-pci/include \
                  $(PKGDIR)/server/lib-pci/include/dummies \
                  $(PKGDIR)/server/include . \
                  $(LINUX24_INCDIR)

CPPFLAGS        = -D__KERNEL__ -Dprintk=printf
CFLAGS          = -fno-strict-aliasing
WARNINGS        = -Wall -Wstrict-prototypes

SRC_C           += arch-i386/pci-i386.c arch-i386/pci-irq.c arch-i386/pci-pc.c

SRC_C           += pci/pci.c pci/quirks.c pci/compat.c pci/setup-res.c \
                   pci/names.c

include $(L4DIR)/mk/lib.mk

pci/names.o: devlist.h classlist.h

vpath pci.ids $(PKGDIR)/server/lib-pci/src/pci
devlist.h classlist.h: pci.ids gen-devlist
	$(VERBOSE)echo -e "  ... Generating devlist.h/classlist.h"
	$(VERBOSE)./gen-devlist < $<

gen-devlist: pci/gen-devlist.c .general.d
	@$(BUILD_MESSAGE)
	$(VERBOSE)$(HOST_CC) -o $@ $<

clean::
	$(VERBOSE)$(RM) $(addprefix $(PKGDIR)/server/lib-pci/src/pci,\
	                pci/devlist.h pci/classlist.h pci/gen-devlist)

ifeq ($(LINUX24_INCDIR),)
  $(error Please set setup path to Linux 2.4.27 includes (LINUX24_INCDIR).)
endif
