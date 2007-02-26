PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= host
TARGET		= nh_setkey
#DEFAULT_RELOC	= 0x01000000
#LIBCLIBS        = -nostdlib -lmc -loskit_support $(dir $(GCCLIB))libsupc++.a $(GCCLIB) $(GCCLIB_EH) -lc++support
#LIBCLIBS        = -nostdlib -lmc -loskit_support $(GCCLIB)
L4LIBS		= -lnethub \
                  -lcxx_start -lcxx_thread -lcxx_io -lcxx_util     \
		  -lcxx_linux_io -lcxx_base -ll4sys -lnames $(ROOTLIB) \
		  -ll4util

PRIVATE_INCDIR   = $(L4INCDIR)
PRIVATE_LIBDIR   = $(L4LIBDIR)

SYSTEMS		= x86-l4v2 x86-l4x0

SRC_CC		= main.cc

CPPFLAGS 	+= -DLOG_TAG=\"nh_key\"
CXXFLAGS	+= -fno-exceptions -fno-rtti

include $(L4DIR)/mk/prog.mk

