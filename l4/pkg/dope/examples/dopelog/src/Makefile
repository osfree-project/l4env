PKGDIR        ?= ..
L4DIR         ?= $(PKGDIR)/../../../..

TARGET         = $(PKGNAME)

DEFAULT_RELOC  = 0x00400000

SRC_C          = main.c stringlist.c logserv.c window.c log_term.c

PRIVATE_INCDIR = .

LIBS           = -ll4dope

include $(L4DIR)/mk/prog.mk

window.o: window.i

window.i: $(PKGDIR)/src/window.dpe
	$(L4DIR)/pkg/dope/tool/gen_dopecode $^ > $@
