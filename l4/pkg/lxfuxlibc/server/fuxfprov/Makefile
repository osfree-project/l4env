PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= fuxfprov
MODE		= sigma0
DEFAULT_RELOC	= 0x01000000
SYSTEMS		= x86-l4v2 x86-l4x0

L4LIBS		= -lmain -lgeneric_fprov-server -ldm_generic -ldm_mem \
		  -ll4env_err -ll4sys -lnames -lrmgr -ll4util
SRC_C		= main.c l4env_dummies.c
LIBS		= -lclxfux.o

include $(L4DIR)/mk/prog.mk

