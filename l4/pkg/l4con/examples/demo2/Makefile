PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= con_demo2
LIBS		= -lcon
DEFAULT_RELOC_x86 = 0x00800000
DEFAULT_RELOC_arm = 0x00370000
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

SRC_C		= demo2.c
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/prog.mk
