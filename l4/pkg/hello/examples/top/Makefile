PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= fiasco_top
SYSTEMS		= x86-l4v2
MODE		= tiny
DEFAULT_RELOC	= 0x01000000

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk

L4LIBS		+= -ll4util_root -lsigma0 $(ROOTLIB)
