#
# Makefile for sources of log server, no net version
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET		= log
DEFAULT_RELOC	= 0x00300000
MODE		= sigma0
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= logserver.c stuff.c flusher.c
VPATH		= $(PKGDIR)/server/src
L4LIBS		= -static -lnames -ll4util -lrmgr -llog $(GCCLIB)

CPPFLAGS 	= -DLOG_TAG='"$(TARGET)"' -DCONFIG_USE_TCPIP=0

include $(L4DIR)/mk/prog.mk
