PKGDIR       ?= ../..
L4DIR        ?= $(PKGDIR)/../..
TARGET        = launchpad 
DEFAULT_RELOC = 0x00a80000
SRC_C         = main.c
LIBS          = -ll4dope -lloaderif

DEPENDS_PKGS  = dope

include $(L4DIR)/mk/prog.mk
