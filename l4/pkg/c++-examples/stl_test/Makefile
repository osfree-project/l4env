PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

MODE		:= sigma0
DEFAULT_RELOC_x86 = 0x01000000
DEFAULT_RELOC_arm = 0x00100000

include $(L4DIR)/mk/Makeconf

# Is only supposed to work with uclibc ...
ifeq ($(USE_UCLIBC),y)

CXXLIB		= $(shell $(CXX) -print-file-name=libsupc++.a)

GCCLIB		:= $(GCCLIB_EH) $(GCCLIB) 
SRC_CC		:= main.cc
TARGET		:= c++-stl-test
SYSTEMS		= x86-l4v2 arm-l4x0

CPPFLAGS 	+= -DLOG_TAG=\"cxxtest\"

include $(L4DIR)/mk/prog.mk

LIBCLIBS	+= -ll4util
L4LIBS		+= -luc_c++ -lc++start.o $(CXXLIB) -lmain -lcxx_base

CFLAGS		:= -g
CXXFLAGS	:= -g

else
all::
	@echo -e $(EMPHSTART)"stl_test: STL only builds with uClibc."$(EMPHSTOP)
	
ifeq ($(filter clean cleanall,$(MAKECMDGOALS)),)
TARGET          =
endif

include $(L4DIR)/mk/prog.mk
endif
