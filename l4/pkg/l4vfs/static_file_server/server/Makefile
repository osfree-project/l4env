# directories we need to know
PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../../..

FILES        := $(shell cat $(PKGDIR)/server/files.lst)
FILES_ofs    := $(addsuffix .ofs,$(FILES))

SYSTEMS       = x86-l4v2 x86-l4x0

SRC_C         = main.c basic_io.c resolve.c state.c

TARGET        = static_file_server
MODE          = l4env_minimal
DEFAULT_RELOC = 0x01100000
OBJS         += $(FILES_ofs)
LIBS         += -ll4vfs_name_server -ll4vfs_name_space_provider  \
                -ll4vfs_object_server-server

include $(L4DIR)/mk/prog.mk

%.ofs: %
	@$(COMP_MESSAGE)
	$(VERBOSE)$(LD) -r --oformat default -o $@ -b binary $^

$(FILES): %: $(PKGDIR)/server/%
	@ln -s $? $@
