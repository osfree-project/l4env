PKGDIR = ..
L4DIR  ?= $(PKGDIR)/../..

SRC_DOX_GUIDE	= dsi.cfg
SRC_TEX		= signalling.tex
SRC_FIG		= arch.fig sig_discussion.fig sig_sync_ex.fig

MAG_DEFAULT	= 1.3
MAG_arch	= 1
MAGNIFIER = $(firstword $(MAG_$*) $(MAG_DEFAULT))

FIG2PNG_PROG	= fig2dev -L png -m $(MAGNIFIER)

include $(L4DIR)/mk/doc.mk
