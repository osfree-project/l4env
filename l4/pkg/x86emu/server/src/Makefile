PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= x86emu
MODE		= sigma0
DEFAULT_RELOC	= 0x00F00000
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= emu.c
PRIVATE_INCDIR	= ../../include
CFLAGS		= -fomit-frame-pointer
LIBS		= -lx86emu

DEBUG		= 0

include $(L4DIR)/mk/prog.mk
