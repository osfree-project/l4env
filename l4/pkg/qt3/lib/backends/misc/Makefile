PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

PRIVATE_INCDIR	= $(L4DIR)/include/qt3

TARGET		= libqt3_misc.a

CXXFLAGS	= -fno-rtti -fno-exceptions

MODE		= l4env_base
SYSTEMS		= x86-l4v2

#DEBUG		= 1

SRC_C_libqt3_misc.a		= l4qws_log.c fakesyms.c detect_flips.c
SRC_CC_libqt3_misc.a		= new_delete.cc

include $(L4DIR)/mk/lib.mk
