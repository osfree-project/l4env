SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
vpath %.c	$(PKGDIR)/server/src/
vpath %.S	$(PKGDIR)/server/src/
DEFINES		+= -DFIASCO_UX

TARGET		= pingpong-ux
MODE		= sigma0
DEFAULT_RELOC	= 0x00800000

SRC_C		= ipc_buffer.c worker_sysenter.c worker_int30.c \
		  pingpong.c global.c helper.c helper_sysenter.c \
		  helper_int30.c flooder.c
SRC_S		= exception.S ipc_sysenter.S ipc_int30.S
L4LIBS		= -static -lmain -lparsecmdline -ll4util -lrmgr \
		  $(GCCLIB) -ll4sys

include $(L4DIR)/mk/prog.mk
