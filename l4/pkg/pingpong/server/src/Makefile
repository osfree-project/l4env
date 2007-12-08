PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= pingpong
MODE		= tiny
DEFAULT_RELOC	= 0x00800000
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

SRC_C		= ipc_buffer.c \
		  helper.c \
		  pingpong.c memcpy.c global.c flooder.c \
		  instructions.c

CFLAGS_x86-l4v2	= -DBENCH_x86=1
CFLAGS_arm-l4v2	= -DBENCH_GENERIC=1
CFLAGS_amd64-l4v2 = -DBENCH_GENERIC=1

GENERIC_SRC     = helper_generic.c worker_generic.c

ifeq ($(CFLAGS_x86-l4v2),-DBENCH_x86=1)
SRC_C_x86-l4v2	= worker_sysenter.c worker_int30.c \
		  helper_sysenter.c helper_int30.c \
		  worker_kipcalls.c helper_kipcalls.c
SRC_S_x86-l4v2	= ipc_sysenter.S ipc_int30.S ipc_kipcalls.S
else
SRC_C_x86-l4v2  = $(GENERIC_SRC)
endif

SRC_C_arm-l4v2  = $(GENERIC_SRC)
SRC_C_amd64-l4v2= $(GENERIC_SRC)

L4LIBS		= -static -lmain -lparsecmdline -ll4util -lsigma0 \
		  $(GCCLIB) -lroot -ll4sys -ll4util_root

include $(L4DIR)/mk/prog.mk
