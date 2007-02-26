# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Template for directories containing only subdirs
#
# 05/2002 Jork Loeser <jork.loeser@inf.tu-dresden.de>

include $(L4DIR)/mk/Makeconf

ifeq ($(PKGDIR),.)
TARGET ?= $(patsubst %/Makefile,%,$(wildcard $(addsuffix /Makefile, \
	idl include src lib server examples doc)))
$(if $(wildcard include/Makefile), idl lib server examples: include)
$(if $(wildcard idl/Makefile), lib server examples: idl)
$(if $(wildcard lib/Makefile), server examples: lib)
else
TARGET ?= $(patsubst %/Makefile,%,$(wildcard $(addsuffix /Makefile, \
	idl src lib server examples doc)))
endif
SUBDIR_TARGET	:= $(if $(filter doc,$(MAKECMDGOALS)),$(TARGET),    \
			$(filter-out doc,$(TARGET)))

all::	$(SUBDIR_TARGET)
idl include lib server examples doc:
install::

clean cleanall scrub::
	$(VERBOSE)set -e; $(foreach d,$(TARGET),\
		test -f $d/broken || \
		$(MAKE) -C $d $@ $(MKFLAGS) $(MKFLAGS_$(d)); )

install oldconfig reloc txtconfig relink::
	$(VERBOSE)set -e; $(foreach d,$(TARGET),\
		test -f $d/broken -o -f $d/obsolete || \
		$(MAKE) -C $d $@ $(MKFLAGS) $(MKFLAGS_$(d)); )

$(SUBDIR_TARGET):
	$(VERBOSE)test -f $@/broken -o -f $@/obsolete || \
		$(MAKE) -C $@ $(MKFLAGS)

install-symlinks:
	$(warning target install-symlinks is obsolete. Use 'include' instead (warning only))
	$(VERBOSE)$(MAKE) include

help::
	@echo "  all            - build subdirs: $(SUBDIR_TARGET)"
	$(if $(filter doc,$(TARGET)), \
	@echo "  doc            - build documentation")
	@echo "  scrub          - call scrub recursively"
	@echo "  clean          - call clean recursively"
	@echo "  cleanall       - call cleanall recursively"
	@echo "  install        - build subdirs, install recursively then"
	@echo "  oldconfig      - call oldconfig recursively"
	@echo "  reloc          - call reloc recursively"
	@echo "  txtconfig      - call txtconfig recursively"

.PHONY: $(TARGET) all clean cleanall help install oldconfig reloc txtconfig
