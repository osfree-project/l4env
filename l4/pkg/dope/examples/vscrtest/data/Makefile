PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..
SYSTEMS        = x86-l4v2 x86-linux amd64-l4v2
TARGET         = libdata.a
OBJS           = bumpmap.o light.o
INSTALL_TARGET =

vpath %.xga $(SRC_DIR)

include $(L4DIR)/mk/lib.mk

%.o: %.xga
	@$(COMP_MESSAGE)
	$(VERBOSE)outputto=$$(pwd)/$@;                                    \
	   (cd $(SRC_DIR) && \
	    $(LD) -r --oformat default -o $$outputto -b binary $(notdir $^))

