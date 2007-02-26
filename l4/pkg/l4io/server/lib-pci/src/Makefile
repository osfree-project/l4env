PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libpci24.a
SYSTEMS		= x86-l4v2
NOTARGETSTOINSTALL = 1
SRC_C		= glue.c 

### specialties
PRIVATE_INCDIR	= $(PKGDIR)/server/lib-pci/include \
		  $(PKGDIR)/server/lib-pci/include/dummies \
		  $(PKGDIR)/server/include . \
		  $(PKGDIR)/server/lib-pci/linux-headers

CPPFLAGS 	= -D__KERNEL__ -Dprintk=printf
WARNINGS	= -Wall -Wstrict-prototypes

vpath % $(PKGDIR)/server/lib-pci/src/arch-i386
SRC_C		+= pci-i386.c pci-irq.c
SRC_S		= pci-pc.l4.S

vpath % $(PKGDIR)/server/lib-pci/src/pci
SRC_C		+= pci.c quirks.c compat.c setup-res.c names.c

### act the role of ...
include $(L4DIR)/mk/lib.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif

### special rules
pci-pc.l4.S: pci-pc.S .general.d
	$(VERBOSE)echo -e "  ... Substituting lcalls in $(@:.o=.S)"
	$(VERBOSE)sed -e "s/lcall /push %cs; call */g" $< > $@

names.o: devlist.h classlist.h

devlist.h classlist.h: pci.ids gen-devlist
	$(VERBOSE)echo -e "  ... Generating devlist.h/classlist.h"
	$(VERBOSE)./gen-devlist < $<

gen-devlist: gen-devlist.c .general.d
	$(BUILD_MESSAGE)
	$(VERBOSE)$(CC) -o $@ $<

clean::
	$(VERBOSE)$(RM) $(addprefix $(PKGDIR)/server/lib-pci/src/pci,\
			pci/devlist.h pci/classlist.h pci/gen-devlist)
	$(VERBOSE)$(RM) $(PKGDIR)/server/lib-pci/src/arch-i386/pci-pc.S

