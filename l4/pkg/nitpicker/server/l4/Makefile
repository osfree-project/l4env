SYSTEMS          := x86-l4v2 x86-l4x0
PKGDIR           ?= ../..
L4DIR            ?= $(PKGDIR)/../..
DOPEDIR          ?= $(L4DIR)/pkg/dopeng

TARGET            = nitpicker nitpicker-ux
DEFAULT_RELOC     = 0x01b00000

LIBS_ALL          = -ldm_phys -lio -lomega0 -ll4env -levents
LIBS_nitpicker    = -linput $(LIBS_ALL)
LIBS_nitpicker-ux = -linput-ux $(LIBS_ALL)

PRIVATE_INCDIR    = $(PKGDIR)/server/include \
                    $(PKGDIR)/server/idl/OBJ-$(ARCH)-$(L4API) \
                    $(DOPEDIR)/server/gfx \
                    $(L4DIR)/tool/dice/include

SRC_C             = screen.c main.c startup.c view.c buffer.c clipping.c \
                    gfx16.c font.c server.c input.c client.c

OBJS             += default_fnt.o

vpath % $(PKGDIR)/server/common
vpath default.fnt $(DOPEDIR)/server/gfx

CLIENTIDL         = nitpicker.idl nitevent.idl
SERVERIDL         = nitpicker.idl

include $(L4DIR)/mk/prog.mk

%_fnt.o: %.tff
	$(VERBOSE)ld -r --oformat default -o $@ -b binary $^

%.tff: %.fnt
	$(VERBOSE)$(PKGDIR)/tool/fnt2tff/OBJ-$(SYSTEM)/fnt2tff $< $@

