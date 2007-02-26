PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2000000

TARGET		= qt3_top
MODE		= l4env_base

SRC_CC		= main.cc processviewer.cc
#SRC_MOC		= processviewer.h

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
