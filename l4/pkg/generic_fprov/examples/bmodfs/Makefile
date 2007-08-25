PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

TARGET		= bmodfs
SRC_C		= main.c dm.c
LIBS		= -lparsecmdline -lgeneric_fprov-server \
		  -ldm_mem-server -ldm_generic-server
DEFAULT_RELOC   = 0x01220000
DEFAULT_RELOC_arm = 0x00210000

include $(L4DIR)/mk/prog.mk
