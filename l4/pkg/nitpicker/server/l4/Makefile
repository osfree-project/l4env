SYSTEMS          := x86-l4v2
PKGDIR           ?= ../..
L4DIR            ?= $(PKGDIR)/../..
DOPEDIR          ?= $(L4DIR_ABS)/pkg/dope

include $(L4DIR)/mk/Makeconf

TARGET            = nitpicker
DEFAULT_RELOC     = 0x01b00000

LIBS              = -ldm_phys -linput -lio -lomega0 -ll4env -levents

PRIVATE_INCDIR    = $(SRC_DIR)/../include \
                    $(DOPEDIR)/server/gfx

SRC_C             = screen.c main.c startup.c view.c buffer.c clipping.c \
                    gfx16.c font.c server.c input.c client.c

OBJS             += default_fnt.o

vpath % $(SRC_DIR)/../common
vpath default.fnt $(DOPEDIR)/server/gfx

$(warning default.fnt $(DOPEDIR)/server/gfx)

CLIENTIDL         = nitpicker.idl nitevent.idl
SERVERIDL         = nitpicker.idl
IDL_PKGDIR        = $(PKGDIR_OBJ) $(PKGDIR_OBJ)/server

include $(L4DIR)/mk/prog.mk

%_fnt.o: %.tff
	$(VERBOSE)$(LD) -r --oformat default -o $@ -b binary $^

%.tff: %.fnt
	$(VERBOSE)$(PKGDIR_OBJ)/tool/fnt2tff/OBJ-$(SYSTEM)/fnt2tff $< $@

