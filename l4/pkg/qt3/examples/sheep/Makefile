PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2800000

TARGET		= qt3_sheep
MODE		= l4env_base

PRIVATE_INCDIR	= ../english
SRC_CC		= main.cc gamelib.cc window.cc levels.cc wiese.cc
SRC_MOC		= gamelib.h window.h wiese.h
SRC_UI_IMG	= images/bg_0000.png images/bg_0001.png images/bg_0002.png images/bg_0003.png \
                  images/bg_0004.png images/bg_0005.png images/bg_0006.png images/bg_0007.png \
                  images/bg_0008.png images/bg_0009.png images/bg_0010.png images/bg_0011.png \
                  images/bgr.png images/birke.png images/cyclam_0000.png images/cyclam_0001.png \
    		  images/cyclam_0002.png images/cyclam_0003.png images/cyclam.png images/gover.png \
                  images/header.png images/headet.png images/pause.png images/pfeil_0000.png \
                  images/pfeil_0001.png images/pfeil_0002.png images/pfeil_0003.png images/pott.png \
                  images/reispapier_weiss.png images/schaf_0000.png images/schaf_0001.png \
                  images/schaf_0002.png images/schaf_0003.png images/schaf_0004.png \
                  images/schaf_0005.png images/schaf_0006.png images/schaf_0007.png \
                  images/schaf_0008.png images/schaf_0009.png images/schaf_0010.png \
                  images/schaf_0011.png images/schaf_0012.png images/schaf_0013.png \
                  images/schaf_0014.png images/schaf_0015.png images/schaf_0016.png \
                  images/schaf_0017.png images/schaf_0018.png images/schaf_0019.png \
                  images/schaf_0020.png images/schaf_0021.png images/schaf_0022.png \
                  images/schaf_0023.png images/title_french.png images/transporter.png \
                  images/ueberschrift.png images/wespe_0000.png images/wespe_0001.png \
                  images/wespe_0002.png images/wespe_0003.png images/wiese.png

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
