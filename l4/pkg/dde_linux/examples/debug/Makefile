PKGDIR     ?= ../..
L4DIR      ?= $(PKGDIR)/../..

TARGET      = dde_debug
DEFAULT_RELOC = 0x00800000
SYSTEMS     = x86-l4v2

SRC_C       = debug.c

### specialties
PRIVATE_INCDIR = $(PKGDIR)/include/ARCH-$(ARCH) \
                 $(LINUX24_INCDIR)

LIBS = -ldde_linux -lio -lomega0

DEFINES = -D__KERNEL__ -DDDE_LINUX
WARNINGS = -Wall -Wstrict-prototypes

### act the role of ...
include $(L4DIR)/mk/prog.mk

ifeq ($(LINUX24_INCDIR),)
  $(error Please set setup path to Linux 2.4.20 headers (LINUX24_INCDIR).)
endif

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif
