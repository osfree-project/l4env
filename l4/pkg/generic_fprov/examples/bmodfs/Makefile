PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

TARGET		= bmodfs
SRC_C		= main.c dm.c
LIBS		= -lparsecmdline -lgeneric_fprov-server \
		  -ldm_mem-server -ldm_generic-server
DEFAULT_RELOC_x86 = 0x01220000
DEFAULT_RELOC_arm = 0x00260000

include $(L4DIR)/mk/prog.mk
