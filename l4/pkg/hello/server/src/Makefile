PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu

TARGET		= hello
MODE		= tiny

DEFAULT_RELOC_x86 = 0x01000000
DEFAULT_RELOC_arm = 0x00200000
DEFAULT_RELOC_amd64 = 0x01000000

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
