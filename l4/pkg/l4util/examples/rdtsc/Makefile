PKGDIR 		?= ../..
L4DIR		?= $(PKGDIR)/../..

SRC_C		= main.c
MODE		= sigma0
TARGET		= rdtsc
SYSTEMS		= x86-l4v2 x86-l4x0
DEFAULT_RELOC	= 0x00350000
L4LIBS		= -static -lmain -ll4util

include $(L4DIR)/mk/prog.mk
