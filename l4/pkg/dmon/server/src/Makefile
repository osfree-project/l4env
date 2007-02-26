PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET = $(PKGNAME)
DEFAULT_RELOC = 0x00400000

SRC_C = dmon.c dumper.c logger.c
DPE_dmon.c = dmon.dpi

LIBS = -ll4dope -lvscreen -levents -lloaderif -lvt100

ifneq ($(WITH_PETZE),)
 LIBS += -lpetze
 DEFINES += -DWITH_PETZE=1
endif
ifneq ($(WITH_TMPFS),)
 LIBS += -ltmpfs
 DEFINES += -DWITH_TMPFS=1
endif

PRIVATE_INCDIR = .

include $(L4DIR)/mk/prog.mk
