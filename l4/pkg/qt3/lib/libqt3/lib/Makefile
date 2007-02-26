PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS = x86

TGT 	= libqt3_embedded.a
TGT_DBG	= libqt3_embedded_dbg.a

all:: $(TGT) $(TGT_DBG)

INSTALL_TARGET = $(TGT) $(TGT_DBG)

$(TGT):
	$(STRIP) -d -o $@ $(PKGDIR)/lib/libqt3/lib/$@

$(TGT_DBG):
	$(LN) -s $(PKGDIR)/lib/libqt3/lib/$(TGT) $@

cleanall::
	rm -rf libqt3_embedded.la libqt3_embedded.prl pkgconfig qt3_embedded.pc

include $(L4DIR)/mk/lib.mk
