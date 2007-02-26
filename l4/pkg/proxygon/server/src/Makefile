SYSTEMS       := x86-l4v2
PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..
TARGET         = proxygon
DEFAULT_RELOC  = 0x00b00000
LIBS           = -ll4dope -lvscreen -ll4con-idl -levents

SRC_C          = main.c if.c vc.c pslim.c events.c dummies.c lat0_12.c
PRIVATE_INCDIR = $(PKGDIR)/server/include

include $(L4DIR)/mk/prog.mk
