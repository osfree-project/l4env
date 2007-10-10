PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2 amd64-l4v2 arm-l4v2
TARGET = $(PKGNAME)
DEFAULT_RELOC = 0x00400000

# use dietlibc_r
WANT_THREADSAFE = 1

SRC_C = dmon.c dumper.c logger.c
DPE_dmon.c = dmon.dpi

LIBS = -ll4dope -lvscreen -levents -lloaderif -lvt100 -lc_be_select -ll4vfs_select

ifneq ($(WITH_PETZE),)
 LIBS += -lpetze
 DEFINES += -DWITH_PETZE=1
endif
ifneq ($(WITH_TMPFS),)
 LIBS += -ltmpfs
 DEFINES += -DWITH_TMPFS=1
endif

PRIVATE_INCDIR = .

SERVERIDL	= log.idl
IDL_PKGDIR	= $(PKGDIR_OBJ)/../log

include $(L4DIR)/mk/prog.mk
