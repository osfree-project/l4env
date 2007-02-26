# directories we need to know
PKGDIR         ?= ../..
L4DIR          ?= $(PKGDIR)/../..

SYSTEMS         = x86-l4v2

PRIVATE_INCDIR  =+ $(PKGDIR)/examples/common   \
                   $(PKGDIR)/examples/dope_mon \
                   .
vpath %          $(PKGDIR)/examples/common

SRC_C           = main.c vis.c wins.c color.c

DEFAULT_RELOC   = 0x0062b000
TARGET          = rt_mon_dope

MODE            = l4env_minimal

LIBS           += -lrt_mon_mon -lrt_mon_client -ll4dope -lvscreen \
                  -lvfb_drawing -lparsecmdline

DEPENDS_PKGS    = libvfb

include $(L4DIR)/mk/prog.mk

vpath main_window.dpe $(PKGDIR)/examples/dope_mon
vpath main_window.dpe $(PKGDIR)/examples/dope_mon/OBJ-$(SYSTEM)

main_window.i: main_window.dpe
	$(L4DIR)/tool/gen_dopecode/gen_dopecode $^ > $@

main.o: main_window.i
