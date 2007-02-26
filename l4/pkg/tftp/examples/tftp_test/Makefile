PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= tftp_test
DEFAULT_RELOC	= 0x00A00000
SYSTEMS		= x86-l4v2

SRC_C		= main.c
LIBS 		= -lgeneric_fprov

include $(L4DIR)/mk/prog.mk
