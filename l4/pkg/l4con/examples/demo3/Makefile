PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= con_demo3
LIBS		= -lcon
DEFAULT_RELOC_x86 = 0x00800000
DEFAULT_RELOC_arm = 0x00350000
SYSTEMS		= x86-l4v2 arm-l4v2

SRC_C		= demo3.c
PRIVATE_INCDIR	= $(SRC_DIR)/../include

include $(L4DIR)/mk/prog.mk
