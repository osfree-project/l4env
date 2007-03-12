PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

MODE		:= l4env
DEFAULT_RELOC_x86 = 0x01000000
DEFAULT_RELOC_arm = 0x00100000

include $(L4DIR)/mk/Makeconf

# Is not supposed to work with OSKit ...
ifneq ($(USE_OSKIT),y)

CXXLIB		= $(shell $(CXX) -print-file-name=libsupc++.a)

GCCLIB		:= $(GCCLIB_EH) $(GCCLIB) 
SRC_CC		:= main.cc
TARGET		:= c++-excpt-test
SYSTEMS		= x86-l4v2 #arm-l4x0

CPPFLAGS 	+= -DLOG_TAG=\"cxxtest\"

include $(L4DIR)/mk/prog.mk

L4LIBS		+= -lc++start.o $(CXXLIB) -lmain -lcxx_base

CFLAGS		:= -g
CXXFLAGS	:= -g

else
include $(L4DIR)/mk/prog.mk
endif
