
PKGDIR         ?= ../..
L4DIR          ?= $(PKGDIR)/../..
SYSTEMS       = x86-l4v2
TARGET          = presenter_l4vfs_fprov
SRC_C		= main.c
SERVERIDL	= presenter_fprov.idl
LIBS		= -ldm_generic -llogserver
DEFAULT_RELOC = 00d00000
MODE		= l4env_base

include $(L4DIR)/mk/prog.mk
