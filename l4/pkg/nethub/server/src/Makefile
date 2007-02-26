PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= nethub
DEFAULT_RELOC	= 0x01000000
L4LIBS		= -ll4util -ll4sys -lnames -llog -lrmgr \
		  -lcxx_start -lcxx_thread -lcxx_io -lcxx_io_kdebug -lcxx_base

SYSTEMS		= x86-l4v2 x86-l4x0

SRC_CC		= main.cc pipethread.cc

CPPFLAGS 	+= -DLOG_TAG=\"nethub\"
CXXFLAGS	+= -fno-rtti -fno-exceptions

include $(L4DIR)/mk/prog.mk
