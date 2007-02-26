#
# Makefile for sources of log server, net version
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET		= log_net
DEFAULT_RELOC	= 0x00300000
MODE		= oskit10_sigma0

SRC_C		= logserver.c stuff.c flusher.c tcpip.c muxed.c
VPATH		= $(PKGDIR)/server/src

# we do not link logserver within our standard libs!
L4LIBS		= -static -lnames -ll4util -lrmgr -llog $(GCCLIB)

# we need a slightly different library order

LIBCLIBS	= -nostdlib -loskit10_support -loskit_clientos -loskit_bootp \
		  -loskit_linux_dev -loskit_freebsd_net -loskit10_support \
		  -loskit_kern -loskit_lmm -loskit_amm -loskit_c \
		  -loskit_kern

CPPFLAGS 	= -DLOG_TAG='"$(TARGET)"' -DCONFIG_USE_TCPIP=1

include $(L4DIR)/mk/prog.mk
