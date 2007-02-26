PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dm_phys_test
DEFAULT_RELOC	= 0x01600000
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2

LIBS		= -ldm_phys -levents
SRC_C		= main.c
DEBUG		= 1

include $(L4DIR)/mk/prog.mk
