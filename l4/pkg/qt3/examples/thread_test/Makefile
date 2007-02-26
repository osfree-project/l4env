PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x2000000

TARGET		= qttest_thread_test
MODE		= l4env_base
QT3_IO_BACKEND	= con # one of con, dope, sdl

SRC_CC		= main.cc

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
