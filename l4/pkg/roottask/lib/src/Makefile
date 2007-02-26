PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libroot.a
CLIENTIDL	= rmgr.idl
MODE		= tiny
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2 x86-l4secv2emu
SRC_C		= libroot.c
CFLAGS		+= -ffunction-sections

include $(L4DIR)/mk/lib.mk
