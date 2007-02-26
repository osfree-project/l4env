PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= exec_test
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0
SRC_C		= main.c
DEFAULT_RELOC	= 0x00A00000
LIBS		+= -ll4exec

include $(L4DIR)/mk/prog.mk
