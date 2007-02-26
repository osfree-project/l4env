SYSTEMS      := x86-l4v2 x86-l4x0
PKGDIR       ?= ../../..
L4DIR        ?= $(PKGDIR)/../..

TARGET        = nitovlwm
DEFAULT_RELOC = 0x00b40000

LIBS          = -lovlsrv-idl

SRC_C         = main.c
CLIENTIDL     = nitpicker.idl
SERVERIDL     = nitevent.idl

include $(L4DIR)/mk/prog.mk
