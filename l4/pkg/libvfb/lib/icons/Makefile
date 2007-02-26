PKGDIR    ?= ../..
L4DIR     ?= $(PKGDIR)/../..

SRC_C      = icons.c

MODE       = l4env_minimal
SYSTEMS    = x86-l4v2
TARGET     = libvfb_icons.a

FILES_DIR ?= $(PKGDIR)/lib/icons/images
FILES      = tree_corner.png tree_cross.png tree_minus.png tree_none.png  \
             tree_plus.png tree_vert.png                                  \
             folder_grey.png misc.png unknown.png filetype_vt.png         \
             filetype_dpe.png filetype_txt.png
#FILES     := $(wildcard $(FILES_DIR)/*.png)

OBJS      += $(FILES:.png=.o)

include $(L4DIR)/mk/lib.mk

# convert to raw image format
%.raw: %.png
	@$(GEN_MESSAGE)
	$(VERBOSE)convert $< rgba:$*.raw

# convert to object file
%.o: %.raw
	$(VERBOSE)$(LD) -r --oformat default -o $@ -b binary $^

#	$(VERBOSE)$(OBJCOPY) -I binary -O elf32-i386 $^ $@.binary
#	$(VERBOSE)echo 'asm(".section .rodata.image_info;    '\
#	     '_bin_$(notdir $<)_name:            '\
#	     '.ascii \"$(notdir $<)\"; .byte 0;  '\
#	     '.section .image_info;               '\
#	     '.word _binary_$(notdir $<)_start;  '\
#	     '.word _binary_$(notdir $<)_size;   '\
#	     '.word _bin_$(notdir $<)_name; ");' > $@.extra.c
#	$(VERBOSE)$(HOST_CC) -c $@.extra.c
#	$(VERBOSE)$(LD) --no-warn-mismatch -r $@.extra.o $@.binary -o $@

$(FILES): %: $(FILES_DIR)/%
	@ln -fs $? $@

test:
	echo $(FILES)
	echo $(OBJS)
