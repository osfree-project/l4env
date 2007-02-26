PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2000000

TARGET		= qttest_widgets
MODE		= l4env_base
QT3_IO_BACKEND	= con # one of con, dope, sdl

SRC_CC		= main.cc widgets.cc aclock.cc dclock.cc
SRC_MOC		= widgets.h aclock.h dclock.h

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
