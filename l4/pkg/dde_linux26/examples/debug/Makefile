PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dde26_debug
DEFAULT_RELOC	= 0x0900000
SYSTEMS		= x86-l4v2

SRC_C		= debug.c

### specialties
PRIVATE_INCDIR	= $(DDE26_INCDIR) $(LINUX26_INCDIR)

LIBS		= -ldde_linux26 -lio -lomega0 -llist_alloc

DEFINES		= -D__KERNEL__ -DDDE_LINUX
WARNINGS        = -Wall -Wstrict-prototypes

### act the role of ...
include $(L4DIR)/mk/prog.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif
