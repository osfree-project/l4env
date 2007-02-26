PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS				= x86-l4v2
DEFAULT_RELOC_qttest_server	= 0x2000000
DEFAULT_RELOC_qttest_client	= 0x2800000
DEFAULT_RELOC_qttest_client2	= 0x3000000

TARGET		= qttest_server qttest_client
MODE		= l4env_base
QT3_IO_BACKEND	= con # one of con, dope, sdl

SRC_CC_qttest_server	= server.cc
SRC_CC_qttest_client	= client.cc
SRC_CC_qttest_client2	= client.cc

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
