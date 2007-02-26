SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
vpath %.c	$(PKGDIR)/server/src/
vpath %.S	$(PKGDIR)/server/src/
DEFINES		+= -DFIASCO_UX

TARGET		= pingpong-ux
MODE		= sigma0
DEFAULT_RELOC	= 0x00800000

SRC_C		= pingpong.c global.c
SRC_S		= exception.S ipc.S
L4LIBS		= -static -lparsecmdline -ll4util -lrmgr $(GCCLIB) -ll4sys

include $(L4DIR)/mk/prog.mk
