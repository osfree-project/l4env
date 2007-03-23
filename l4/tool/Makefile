L4DIR ?= ..

# Be sure to add a "/" to config to make the target different from
# BID's "config" target
TARGET := $(wildcard config/ gendep preprocess checkbiw)
TARGET_doc := $(wildcard gendep preprocess checkbiw)

all::
doc:
	$(if $(TARGET_doc), $(VERBOSE)set -e ; for d in $(TARGET_doc) ; do \
		PWD=$(PWD)/$$d $(MAKE) -C $$d $@ ; done )

MKFLAGS_dice = -f Makefile.drops

include $(L4DIR)/mk/subdir.mk
