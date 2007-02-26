PKGDIR 		?= ../..
L4DIR		?= $(PKGDIR)/../..

SRC_C		= main.c
MODE		= tiny
TARGET		= rdtsc
SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x00350000
LIBS		= -ll4util_root -lsigma0 $(ROOTLIB)

include $(L4DIR)/mk/prog.mk
