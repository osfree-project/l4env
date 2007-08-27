PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

TARGET		= copyfs
SRC_C		= main.c
LIBS		= -lgeneric_fprov -lgeneric_fprov-server

DEFAULT_RELOC   = 0x01280000
DEFAULT_RELOC_arm = 0x001b1000

include $(L4DIR)/mk/prog.mk
