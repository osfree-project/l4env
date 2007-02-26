# directories we need to know
PKGDIR           ?= ../../
L4DIR            ?= $(PKGDIR)/../../..

SYSTEMS           = x86-l4v2

PRIVATE_INCDIR    = $(PKGDIR)/examples/vttest/         \
                    $(PKGDIR)/examples/vttest/contrib

vpath % $(PKGDIR)/examples/vttest                   \
        $(PKGDIR)/examples/vttest/contrib

DEFINES           = -DHAVE_CONFIG_H -D__L4 -DRELEASE=2

TESTFILES         = main.c ttymodes.c unix_io.c charsets.c color.c           \
                    esc.c keyboard.c mouse.c nonvt100.c printer.c reports.c  \
                    reset.c setup.c sixel.c status.c tek4014.c vt220.c       \
                    vt420.c vt52.c xterm.c

SRC_C_vttest_dope = $(TESTFILES) wrapper_dope.c
SRC_C_vttest_con  = $(TESTFILES) wrapper_con.c

DEFAULT_RELOC     = 0x00200000
TARGET            = vttest_dope vttest_con

MODE              = l4env_base

include $(L4DIR)/mk/prog.mk
