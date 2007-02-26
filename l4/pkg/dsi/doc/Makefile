PKGDIR = ..
L4DIR  = $(PKGDIR)/../..

SRC_DOX_GUIDE	= dsi.cfg
SRC_TEX		= signalling.tex

SRC_FIG		= arch.fig sig_discussion.fig sig_sync_ex.fig
PNG		= $(SRC_FIG:.fig=.png)
EPS		= $(SRC_FIG:.fig=.eps)
MAG_DEFAULT	= 1.3
MAG_arch	= 1

all::
dsi:	$(PNG) $(EPS)

MAGNIFIER = $(firstword $(MAG_$*) $(MAG_DEFAULT))

%.png:  %.fig Makefile
	@$(GEN_MESSAGE)
	$(VERBOSE)fig2dev -L png -m $(MAGNIFIER) $< $@

clean cleanall::
	$(VERBOSE)$(RM) $(EPS) $(PNG)

include $(L4DIR)/mk/doc.mk
