PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libroot.a
MODE		= tiny
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu
SRC_C		= libroot.c
CFLAGS		+= -ffunction-sections

include $(L4DIR)/mk/lib.mk

ifeq ($(USE_TASKLIB),y)
CLIENTIDL	= rmgr_task.idl
else
CLIENTIDL	= rmgr.idl
endif
