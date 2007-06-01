PKGDIR		 ?= ../..
L4DIR		= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 arm-l4v2
TARGET		= loader

WITH_IPC_MON	= n

DEFAULT_RELOC   = 0x01300000
DEFAULT_RELOC_arm = 0x002c0000
PRIVATE_INCDIR	= $(PKGDIR)/server/src .
LIB_IPCMON-y	= -lipcmon
LIBS		= \
		-lgeneric_ts \
		-lgeneric_fprov \
		-lloaderif \
		-ldm_phys \
		-levents \
		-lparsecmdline \
		-ll4util_root \
		$(LIB_IPCMON-$(WITH_IPC_MON))
SRC_C		= main.c cfg-parse.c cfg.c pager.c app.c fprov-if.c dm-if.c \
		  idl.c events.c elf-loader.c
SERVERIDL	= loader.idl
OBJS		= trampoline.s.o

include $(L4DIR)/mk/prog.mk

DEFINES_IPCMON-y = -DIPCMON
DEFINES		+= $(DEFINES_IPCMON-$(WITH_IPC_MON))

DEFINES		+= -DUSE_LDSO

# create Makeconf.local with EMULATE_MMIO = 1 to activate this
ifeq ($(EMULATE_MMIO),1)
SRC_C		+= emulate.c emu_tulip.c emu_ide.c emu_e1000.c
DEFINES		+= -DEMULATE_MMIO
LIBS		+= -lio
endif

cfg-parse.c: cfg-scan.c

WARNINGS_cfg-parse.o = -Wno-unused -Wno-parentheses \
		       -Wno-missing-prototypes -Wno-missing-declarations

