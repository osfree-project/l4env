L4DIR     ?= ../..

DIALOG     = lxdialog
DIALOGDIR  = lxdialog
INSTALLDIR = $(DROPS_STDDIR)/tool/bin

all install:: $(DIALOGDIR)/$(DIALOG)

$(DIALOGDIR)/$(DIALOG):
	$(VERBOSE)PWD=$(PWD)/$(DIALOGDIR) $(MAKE) -C $(DIALOGDIR)

install::
	$(INSTALL) -d $(INSTALLDIR)
	$(INSTALL) Configure Menuconfig $(INSTALLDIR)
	$(INSTALL) -d $(INSTALLDIR)/$(DIALOGDIR)
	$(INSTALL) $(OBJ_DIR)/$(DIALOGDIR)/$(DIALOG) $(INSTALLDIR)/$(DIALOGDIR)/$(DIALOG)

clean cleanall::
	$(VERBOSE)PWD=$(PWD)/$(DIALOGDIR) $(MAKE) -C $(DIALOGDIR) $@

doc::

BID_NEED_CONFIG=
include $(L4DIR)/mk/Makeconf
