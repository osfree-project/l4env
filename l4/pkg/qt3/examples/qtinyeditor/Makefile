PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2800000

TARGET		= qt3_qtinyeditor
MODE		= l4env_base

SRC_CC		= qtinyeditordoc.cc qtinyeditor.cc qtinyeditorview.cc main.cc
SRC_MOC		= qtinyeditordoc.h qtinyeditor.h qtinyeditorview.h
SRC_UI_IMG	= 

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
