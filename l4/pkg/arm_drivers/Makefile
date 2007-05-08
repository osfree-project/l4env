PKGDIR	= .
L4DIR	?= $(PKGDIR)/../..

TARGET  = include lib
SUBDIRS = common uart drv lcd

include $(L4DIR)/mk/subdir.mk

lib: $(addsuffix .lib,$(SUBDIRS))

$(addsuffix .lib,$(SUBDIRS)):
	echo $@
	$(VERBOSE)PWD=$(PWD)/$(subst .lib,,$@) $(MAKE) -C $(subst .lib,,$@) src

.PHONY: lib
