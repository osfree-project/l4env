PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= pingpong
MODE		= sigma0
DEFAULT_RELOC	= 0x00800000
SYSTEMS		= x86-l4v2 x86-l4x0
ifeq ($(L4API),l4v2)
#DEFINES		= -DL4V2_IPC_SYSENTER
endif
ifeq ($(L4API),l4x0)
#DEFINES		= -DL4X0_IPC_SYSENTER
endif

SRC_C		= pingpong.c memcpy.c global.c
SRC_S		= exception.S ipc.S
L4LIBS		= -static -lparsecmdline -ll4util -lrmgr $(GCCLIB) -ll4sys

include $(L4DIR)/mk/prog.mk
