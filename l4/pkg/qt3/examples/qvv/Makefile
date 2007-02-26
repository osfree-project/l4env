PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2800000

TARGET		= qt3_qvv
MODE		= l4env_base

SRC_CC		= qvv.cc
SRC_MOC		= qvv.h
SRC_UI		= overwrite.ui
WARNINGS_qvv.o	= -Wno-switch -Wno-unused

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
