
PKGDIR         ?= ../../..
L4DIR          ?= $(PKGDIR)/../..
TARGET          = presenter_fprov
MODE		= l4linux
SRC_C		= emul_l4rm.c l4env_dummies.c server.c
SERVERIDL	= presenter_fprov.idl
LIBS		= -ldm_generic -llogserver

include $(L4DIR)/mk/prog.mk
