PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2800000

TARGET		= qttest_uic
MODE		= l4env_base
QT3_IO_BACKEND	= con # one of con, dope, sdl

SRC_CC		= main.cc
SRC_UI		= mymainwindow.ui

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
