PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS				= x86-l4v2
DEFAULT_RELOC_qttest_top	= 0x2000000

TARGET		= qttest_top
MODE		= l4env_base
QT3_IO_BACKEND	= con # one of con, dope, sdl

SRC_CC		= main.cc processviewer.cc
#SRC_MOC		= processviewer.h

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
