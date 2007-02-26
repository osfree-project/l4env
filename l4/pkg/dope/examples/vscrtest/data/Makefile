PKGDIR      ?= ../../..
L4DIR       ?= $(PKGDIR)/../..
SYSTEMS      = x86-l4v2 x86-linux x86-l4x0
TARGET       = libdata.a
OBJS         = bumpmap.o light.o

vpath %.xga ..

include $(L4DIR)/mk/lib.mk

%.o: %.xga
	@$(COMP_MESSAGE)
	$(VERBOSE)$(LD) -r --oformat default -o $@ -b binary $^

