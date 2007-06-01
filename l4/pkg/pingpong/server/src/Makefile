PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= pingpong
MODE		= tiny
DEFAULT_RELOC	= 0x00800000
SYSTEMS		= x86-l4v2

SRC_C		= ipc_buffer.c worker_sysenter.c worker_int30.c \
		  helper.c helper_sysenter.c helper_int30.c \
		  pingpong.c memcpy.c global.c flooder.c \
		  instructions.c worker_kipcalls.c helper_kipcalls.c
SRC_S		= ipc_sysenter.S ipc_int30.S ipc_kipcalls.S
L4LIBS		= -static -lmain -lparsecmdline -ll4util -lsigma0 \
		  $(GCCLIB) -ll4sys -lroot -ll4util_root

include $(L4DIR)/mk/prog.mk
