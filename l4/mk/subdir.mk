# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Template for directories containing only subdirs
#
# 05/2002 Jork Loeser <jork.loeser@inf.tu-dresden.de>

# Input: Variable TARGET contains the subdirectories to discover. They
#        are build in the order given in TARGET, even in parallel case
#        (make -j).
#
# targets:
# all relink config install: are maked for every subdir.
#
# scrub, clean, mostlyclean: made for subdirs + scrubbing the local dir.
#
# cleanall realclean: made for subdirs + removing local dependency-files.
#
# $(TARGET): change to the given subdir and do a simple 'make'.

include $(L4DIR)/mk/Makeconf
include $(L4DIR)/mk/config.inc

ifeq ($(PKGDIR),.)
TARGET ?= $(patsubst %/Makefile,%,$(wildcard $(addsuffix /Makefile, \
	idl include src lib server examples doc)))
$(if $(wildcard include/Makefile), idl lib server examples: include)
$(if $(wildcard idl/Makefile), lib server examples: idl)
$(if $(wildcard lib/Makefile), server examples: lib)
.PHONY: idl include lib server examples
else
TARGET ?= $(patsubst %/Makefile,%,$(wildcard $(addsuffix /Makefile, \
	idl src lib server examples doc)))
endif

MKFLAGS += $(MKFLAGS_$@)

install::
all::	$(TARGET)

clean cleanall install oldconfig reloc scrub txtconfig::
	$(if $(TARGET), $(VERBOSE)set -e ; for d in $(TARGET) ; do \
		$(MAKE) -C $$d $@ ; done  )

$(TARGET):
	$(VERBOSE)$(MAKE) -C $@ $(MKFLAGS)

install-symlinks:
	$(warning target install-symlinks is obsolete. Use 'include' instead (warning only))
	$(VERBOSE)$(MAKE) include

help::
	@echo "  all            - build subdirs: $(TARGET)"
	@echo "  scrub          - call scrub recursively"
	@echo "  clean          - call clean recursively"
	@echo "  cleanall       - call cleanall recursively"
	@echo "  install        - build subdirs, install recursively then"
	@echo "  oldconfig      - call oldconfig recursively"
	@echo "  reloc          - call reloc recursively"
	@echo "  txtconfig      - call txtconfig recursively"

.PHONY: $(TARGET) all clean cleanall help install oldconfig reloc txtconfig
