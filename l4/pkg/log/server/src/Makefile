#
# Makefile for sources of log server, no net version
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET		= log
DEFAULT_RELOC_x86 = 0x00400000
DEFAULT_RELOC_arm = 0x00160000
MODE		= sigma0
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

SRC_C		= logserver.c stuff.c flusher.c
VPATH		= $(PKGDIR)/server/src
LIBS_x86	= -ll4serial
LIBS_arm	=
LIBS		= -static -lmain -lnames -ll4util -lparsecmdline \
		  $(LIBS_$(ARCH)) -llog $(GCCLIB)
DEFINES_x86	= -DCONFIG_USE_SERIAL
DEFINES		= $(DEFINES_$(ARCH))

CPPFLAGS 	= -DLOG_TAG='"$(TARGET)"' -DCONFIG_USE_TCPIP=0
PRIVATE_INCDIR	= $(PKGDIR)/lib/include

include $(L4DIR)/mk/prog.mk
