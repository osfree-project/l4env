PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2800000

TARGET		= qttest_application
MODE		= l4env_base
QT3_IO_BACKEND	= con # one of con, dope, sdl

SRC_CC		= main.cc application.cc
SRC_MOC		= application.h

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
