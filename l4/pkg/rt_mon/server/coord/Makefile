# directories we need to know
PKGDIR       ?= ../..
L4DIR        ?=  $(PKGDIR)/../..

SYSTEMS       = x86-l4v2

# source files
SERVERIDL     = rt_mon.idl rt_mon_reg.idl

SRC_C         = coord.c clients.c

MODE          = l4env_minimal
DEFAULT_RELOC = 0x002fd000

TARGET        = rt_mon_coord
LIBS	      = -lparsecmdline

include $(L4DIR)/mk/prog.mk
