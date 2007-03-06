PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= fprov_cat
DEFAULT_RELOC	= 0x00A00000
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2

SRC_C		= main.c
LIBS 		= -lgeneric_fprov -lparsecmdline

include $(L4DIR)/mk/prog.mk
