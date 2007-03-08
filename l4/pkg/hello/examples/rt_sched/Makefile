PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= rt_sched
DEFAULT_RELOC	= 0x01000000
SYSTEMS		= x86-l4v2
SRC_C		= main.c

MODE		= sigma0
L4LIBS		= -lmain -ll4util -ll4sys -llog -lparsecmdline

include $(L4DIR)/mk/prog.mk
