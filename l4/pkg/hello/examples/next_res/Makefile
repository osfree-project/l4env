PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= next_res
DEFAULT_RELOC	= 0x01000000
SYSTEMS		= x86-l4v2
SRC_C		= main.c
MODE		= tiny

include $(L4DIR)/mk/prog.mk

L4LIBS		+= -ll4util_root -lsigma0 $(ROOTLIB)
