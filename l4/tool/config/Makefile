L4DIR     ?= ../..

DIALOG     = lxdialog
DIALOGDIR  = lxdialog
INSTALLDIR = $(DROPS_STDDIR)/tool/bin

all install:: $(DIALOGDIR)/$(DIALOG)

$(DIALOGDIR)/$(DIALOG):
	$(MAKE) -C $(DIALOGDIR) $(DIALOG)

install::
	$(INSTALL) -d $(INSTALLDIR)
	$(INSTALL) Configure Menuconfig $(INSTALLDIR)
	$(INSTALL) -d $(INSTALLDIR)/$(DIALOGDIR)
	$(INSTALL) $(DIALOGDIR)/$(DIALOG) $(INSTALLDIR)/$(DIALOGDIR)/$(DIALOG)

clean cleanall::
	$(MAKE) -C $(DIALOG) $@

doc::

BID_NEED_CONFIG=
include $(L4DIR)/mk/Makeconf
