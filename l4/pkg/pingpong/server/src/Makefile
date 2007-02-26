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

SRC_C		= ipc_buffer.c worker_sysenter.c worker_int30.c \
		  helper.c helper_sysenter.c helper_int30.c \
		  pingpong.c memcpy.c global.c flooder.c \
		  instructions.c
SRC_S		= ipc_sysenter.S ipc_int30.S
L4LIBS		= -static -lmain -lparsecmdline -ll4util $(GCCLIB) -ll4sys

include $(L4DIR)/mk/prog.mk
