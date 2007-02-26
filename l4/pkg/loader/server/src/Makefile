PKGDIR		 ?= ../..
L4DIR		= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 x86-l4x0
TARGET		= loader
DEFAULT_RELOC_x86 = 0x01300000
DEFAULT_RELOC_arm = 0x002c0000
PRIVATE_INCDIR	= $(PKGDIR)/server/src .
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0
LIBS		= \
		-ll4exec \
		-lgeneric_ts \
		-lgeneric_fprov \
		-lloaderif \
		-ldm_phys \
		-levents \
		-lparsecmdline

SRC_C		= main.c cfg-parse.c cfg.c pager.c app.c fprov-if.c dm-if.c \
		  exec-if.c idl.c events.c lib.c elf-loader.c

SERVERIDL	= loader.idl
OBJS		= trampoline.s.o

include $(L4DIR)/mk/prog.mk

# create Makeconf.local with EMULATE_MMIO = 1 to activate this
ifeq ($(EMULATE_MMIO),1)
SRC_C		+= emulate.c emu_tulip.c emu_ide.c emu_e1000.c
DEFINES		+= -DEMULATE_MMIO
LIBS		+= -lio
endif

cfg-parse.c: cfg-scan.c

WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses \
		       -Wno-missing-prototypes -Wno-missing-declarations

