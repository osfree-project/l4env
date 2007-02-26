PKGDIR	= ../..
L4DIR	?= $(PKGDIR)/../..

TARGET		= l4util_test
DEFAULT_RELOC_arm = 0x00A00000
DEFAULT_RELOC_x86 = 0x00A00000

SRC_C		= test.c
MODE		= sigma0
SYSTEMS		= x86-l4v2 arm-l4x0

include $(L4DIR)/mk/prog.mk
