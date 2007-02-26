PKGDIR		 ?= ../..
L4DIR		= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 x86-l4x0
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

# Very experimental!
EMULATE_MMIO	=

SRC_C		= main.c cfg-parse.c cfg.c pager.c app.c fprov-if.c dm-if.c \
		  exec-if.c idl.c

ifneq ($(EMULATE_MMIO),)
SRC_C		+= emulate.c emu_ide.c emu_speedo.c emu_tulip.c
DEFINES		+= -DEMULATE_MMIO
endif

SERVERIDL	= loader.idl
OBJS		= trampoline.s.o

include $(L4DIR)/mk/prog.mk

cfg-parse.c: cfg-scan.c

WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses \
		       -Wno-missing-prototypes -Wno-missing-declarations

