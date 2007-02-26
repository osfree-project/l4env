PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2800000

TARGET		= qt3_ftpclient
MODE		= l4env_base

SRC_CC		= main.cc ftpviewitem.cc
SRC_MOC		= 
SRC_UI		= connectdialog.ui ftpmainwindow.ui
SRC_UI_IMG	= images/file.png images/folder.png

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
