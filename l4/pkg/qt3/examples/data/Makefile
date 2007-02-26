PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..


SHEEP		= english/help.rtf english/about.rtf    \
	  	  gameparm.ini hiscore.ini speicher.ini \
	  	  userlevels.csv hiscore.csv levelfile.csv
SHEEP_SRC	= $(addprefix $(PKGDIR)/examples/sheep/, $(SHEEP))

FONTS		= fontdir helvetica_100_50.qpf helvetica_100_50i.qpf \
	  	  helvetica_100_75.qpf helvetica_180_50.qpf
FONTS_SRC	= $(addprefix $(PKGDIR)/lib/libqt3/lib/fonts/, $(FONTS))

all:: images text fonts sheep

sheep: $(SHEEP_SRC)
	$(VERBOSE)mkdir -p $@
	$(VERBOSE)cp $(SHEEP_SRC) $@

fonts: $(FONTS_SRC)
	$(VERBOSE)mkdir -p $@
	$(VERBOSE)cp $(FONTS_SRC) $@


include $(L4DIR)/mk/subdir.mk
