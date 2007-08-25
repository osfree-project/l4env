PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		:= l4env
SYSTEMS		:= x86-l4v2 amd64-l4v2 arm-l4v2
DEFAULT_RELOC_x86 = 0x01000000

include $(L4DIR)/mk/Makeconf

# Is only supposed to work with uclibc ...
ifeq ($(USE_UCLIBC),y)

CXXLIB		= $(shell $(CXX) -print-file-name=libsupc++.a)

GCCLIB		:= $(GCCLIB_EH) $(GCCLIB) 
SRC_CC		:= components.cc capdescriptor.cc capmanager.cc	\
			   blacklist.cc whitelist.cc
TARGET		:= ipcmon
SERVERIDL   = ipcmon.idl

CPPFLAGS 	+= -DLOG_TAG=\"ipcmon\"

include $(L4DIR)/mk/prog.mk

LIBS		+= -lparsecmdline
LIBCLIBS	+= -ll4util 
L4LIBS		+= -luc_c++ -lc++start.o $(CXXLIB) -lmain -lcxx_base

CFLAGS		+= -g
CXXFLAGS	+= -g -fno-rtti

else
all::
	@echo -e $(EMPHSTART)"ipcmon: This server only builds with uClibc."$(EMPHSTOP)
	
ifeq ($(filter clean cleanall,$(MAKECMDGOALS)),)
TARGET          =
endif

include $(L4DIR)/mk/prog.mk
endif
