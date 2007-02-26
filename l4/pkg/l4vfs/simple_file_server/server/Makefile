# directories we need to know
PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../../..

SYSTEMS       = x86-l4v2 x86-l4x0

# fs files
FILES_DIR     ?= $(PKGDIR)/server/files
FILES_LST     := $(FILES_DIR)/files.lst
FILES         := $(shell cat $(FILES_LST))
FILES_ofs     := $(addsuffix .ofs,$(FILES))

SRC_C          = main.c basic_io.c resolve.c dirs.c arraylist.c state.c

TARGET         = simple_file_server
MODE           = l4env_minimal
DEFAULT_RELOC  = 0x01200000
OBJS          += $(FILES_ofs)
LIBS          += -ll4vfs_name_server -ll4vfs_name_space_provider  \
                 -ll4vfs_mmap_object_server-server                \
                 -ll4vfs_select                                   \
                 -ll4vfs_select_notify-server

SERVERIDL      = simple_file_server.idl

PRIVATE_INCDIR = $(PKGDIR)/server/OBJ-$(SYSTEM)

include $(L4DIR)/mk/prog.mk

%.ofs: %
	@$(COMP_MESSAGE)
	$(VERBOSE)$(LD) -r --oformat default -o $@ -b binary $^

$(FILES): %: $(FILES_DIR)/%
	@ln -fs $? $@

state.o: state.c.decl state.c.inc

state.c.decl state.c.inc: $(FILES_LST)
	@$(GEN_MESSAGE)
	$(VERBOSE)$(PKGDIR)/server/build_static.pl $^ 
