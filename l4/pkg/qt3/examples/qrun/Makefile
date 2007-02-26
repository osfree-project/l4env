PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2800000

TARGET		= qt3_qrun
MODE		= l4env_base

SRC_CC		= main.cc l4.cc
SRC_MOC		=
SRC_UI		= qrunwindow.ui

LIBS		= -lloaderif

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk

# ##############################################
# Linux test case

cleanall::
	$(VERBOSE)test -f Makefile.linux && $(MAKE) -f Makefile.linux distclean || true
	$(VERBOSE)rm -f Makefile.linux

.PHONY: linux
linux:
	$(VERBOSE)make -f Makefile.linux

Makefile.linux: qrun.pro
	$(VERBOSE)qmake -o Makefile.linux


