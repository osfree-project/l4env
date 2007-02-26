PKGDIR?= ../../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS = x86

TGT 		= libqt3_embedded.a
TGT_DBG		= libqt3_embedded_dbg.a

INSTALL_TARGET = $(TGT) $(TGT_DBG)
LIB_QTE_NORMAL	= $(PKGDIR)/lib/libqt3/lib/libqt3_embedded_drops_nodbg.a
LIB_QTE_DBG	= $(PKGDIR)/lib/libqt3/lib/libqt3_embedded_drops.a

all:: $(TGT) $(TGT_DBG)

$(TGT): $(LIB_QTE_NORMAL)
	$(LN) -sf $< $@

$(TGT_DBG): $(LIB_QTE_DBG)
	$(LN) -sf $< $@


$(LIB_QTE_NORMAL): $(LIB_QTE_DBG)
	$(STRIP) -d -o $@ $<

clean::
	rm -rf libqt3_embedded* pkgconfig qt3_embedded.pc

cleanall::
	rm -rf libqt3_embedded* pkgconfig qt3_embedded.pc

include $(L4DIR)/mk/lib.mk
