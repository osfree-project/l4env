PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= abs_timeout_test
SYSTEMS		= x86-l4v2
MODE		= tiny
DEFAULT_RELOC	= 0x01000000
LIBS		= -ll4util_root -lsigma0 -lroot

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
