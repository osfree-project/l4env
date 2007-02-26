# directories we need to know
PKGDIR ?=..
L4DIR  ?=$(PKGDIR)/../../..

SYSTEMS = x86-l4v2

SRC_C   = main.c

TARGET  = fstab
MODE    = l4env_base
DEFAULT_RELOC = 0x01590000

include $(L4DIR)/mk/prog.mk
