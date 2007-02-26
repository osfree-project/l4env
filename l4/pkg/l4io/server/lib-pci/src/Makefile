PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libio-pci24.a
SYSTEMS		= x86-l4v2 x86-l4x0
INSTALL_TARGET	=
SRC_C		= glue.c

### specialties
PRIVATE_INCDIR	= $(PKGDIR)/server/lib-pci/include \
		  $(PKGDIR)/server/lib-pci/include/dummies \
		  $(PKGDIR)/server/include . \
		  $(LINUX24_INCDIR)

CPPFLAGS 	= -D__KERNEL__ -Dprintk=printf
WARNINGS	= -Wall -Wstrict-prototypes

SRC_C		+= arch-i386/pci-i386.c arch-i386/pci-irq.c
SRC_S		= arch-i386/pci-pc.l4.S

SRC_C		+= pci/pci.c pci/quirks.c pci/compat.c pci/setup-res.c \
		   pci/names.c

### act the role of ...
include $(L4DIR)/mk/lib.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif

### special rules
arch-i386/pci-pc.l4.S: arch-i386/pci-pc.S .general.d
	$(VERBOSE)echo -e "  ... Substituting lcalls in $(@:.o=.S)"
	$(VERBOSE)sed -e "s/lcall /push %cs; call */g" $< > $@

pci/names.o: devlist.h classlist.h

vpath pci.ids $(PKGDIR)/server/lib-pci/src/pci
devlist.h classlist.h: pci.ids gen-devlist
	$(VERBOSE)echo -e "  ... Generating devlist.h/classlist.h"
	$(VERBOSE)./gen-devlist < $<

gen-devlist: pci/gen-devlist.c .general.d
	@$(BUILD_MESSAGE)
	$(VERBOSE)$(CC) -o $@ $<

clean::
	$(VERBOSE)$(RM) $(addprefix $(PKGDIR)/server/lib-pci/src/pci,\
			pci/devlist.h pci/classlist.h pci/gen-devlist)
	$(VERBOSE)$(RM) $(PKGDIR)/server/lib-pci/src/arch-i386/pci-pc.S

ifeq ($(LINUX24_INCDIR),)
  $(error Please set setup path to Linux 2.4.20 includes (LINUX24_INCDIR).)
endif
