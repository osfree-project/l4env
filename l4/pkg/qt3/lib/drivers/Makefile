PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

PRIVATE_INCDIR	= $(L4DIR)/include/qt3

TARGET		= libqt3_io_sdl.a libqt3_io_con.a libqt3_io_dope.a

CXXFLAGS	= -fno-rtti -fno-exceptions

MODE		= l4env_base
SYSTEMS		= x86-l4v2

SRC_CC_libqt3_io_sdl.a		= input_sdl.cc output_sdl.cc generated.cc sound.cc
SRC_CC_libqt3_io_con.a		= input_con.cc output_con.cc generated.cc sound.cc
SRC_CC_libqt3_io_dope.a		= input_dope.cc output_dope.cc generated.cc sound.cc

include $(L4DIR)/mk/lib.mk
