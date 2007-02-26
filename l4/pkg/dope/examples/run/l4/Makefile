PKGDIR       ?= ../../..
L4DIR        ?= $(PKGDIR)/../..
TARGET        = doperun
DEFAULT_RELOC = 0x00a80000
SRC_C         = main.c wrapper.c
DEFINES      += -Dprintf=term_printf -Dgetchar=term_getchar
LIBS          = -ll4term -ll4dope -llogserver -lloaderif -ldm_phys

vpath main.c $(L4DIR)/pkg/loader/examples/run

include $(L4DIR)/mk/prog.mk
