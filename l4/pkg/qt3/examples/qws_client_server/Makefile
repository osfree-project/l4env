PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS				= x86-l4v2
DEFAULT_RELOC_qt3_server	= 0x2000000
DEFAULT_RELOC_qt3_client	= 0x2800000

TARGET		= qt3_server qt3_client
MODE		= l4env_base

SRC_CC_qt3_server	= server.cc
SRC_CC_qt3_client	= client.cc

include $(PKGDIR)/mk/qt3.mk
include $(L4DIR)/mk/prog.mk
