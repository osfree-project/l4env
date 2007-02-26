PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

ifneq ($(OBJ_BASE),)
include $(OBJ_BASE)/Makeconf.bid.local
else
ifneq ($(O),)
include $(O)/Makeconf.bid.local
endif
endif


ifneq ($(USE_LDSO),y)
TARGET		= test_dyn_c++
SUBDIRS		= libfoo_dyn libbar_dyn
$(TARGET):	libbar_dyn
endif
MODE		= loader
LIBS		= -lbar_dyn_c++.s -lparsecmdline
DEFAULT_RELOC	= 0x01000000
SRC_CC		= main.cc
CXXFLAGS	= -fno-rtti -fno-exceptions
SYSTEMS		= x86-l4v2 arm-l4x0

# The -E flag is essential here to make sure that _all_ symbols of
# the binary (test_dyn) are exeported as dynamic symbols. We need
# this option since we load a dynamic library after startup
LDFLAGS		= -Wl,-E

all:: $(SUBDIRS)

include $(L4DIR)/mk/prog.mk
