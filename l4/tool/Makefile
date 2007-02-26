L4DIR ?= ..

TARGET := $(wildcard config/ dice flick-obj gcc-wrap gendep preprocess)
TARGET_doc := $(wildcard dice gcc-wrap gendep preprocess)

all::
doc:
	$(if $(TARGET_doc), $(VERBOSE)set -e ; for d in $(TARGET_doc) ; do \
                $(MAKE) -C $$d $@ ; done )

MKFLAGS_dice = -f Makefile.drops

include $(L4DIR)/mk/subdir.mk
