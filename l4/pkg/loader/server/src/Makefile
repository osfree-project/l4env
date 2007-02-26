PKGDIR		 ?= ../..
L4DIR		= $(PKGDIR)/../..
TARGET		= loader
DEFAULT_RELOC	= 0x01300000
PRIVATE_INCDIR	= $(PKGDIR)/server/src .
SYSTEMS		= x86-l4v2 x86-l4x0
LIBS		= \
		-ll4exec \
		-lgeneric_ts \
		-lgeneric_fprov \
		-lloaderif \
		-lcon \
		-ldm_phys

SRC_C		= main.c cfg-parse.c cfg.c \
		  pager.c app.c fprov-if.c dm-if.c idl.c
SERVERIDL	= loader.idl
OBJS		= trampoline.s.o

include $(L4DIR)/mk/prog.mk

cfg-parse.c: cfg-scan.c

WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses

