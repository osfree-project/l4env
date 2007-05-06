PKGDIR	= .
L4DIR	?= $(PKGDIR)/../..

TARGET  = include lib
SUBDIRS = common uart drv lcd

include $(L4DIR)/mk/subdir.mk

clean:: $(addsuffix .clean,$(SUBDIRS))

cleanall:: $(addsuffix .cleanall,$(SUBDIRS))

include: uart.include

lib: uart.lib


# explict dependencies

uart: uart.lib

%.clean: 
	$(VERBOSE)PWD=$(PWD)/$* $(MAKE) -C $* clean

%.cleanall:
	$(VERBOSE)PWD=$(PWD)/$* $(MAKE) -C $* cleanall

$(addsuffix .include,$(SUBDIRS)): %.include: %
	$(VERBOSE)PWD=$(PWD)/$(subst .include,,$@) $(MAKE) -C $(subst .include,,$@) include

$(addsuffix .idl,$(SUBDIRS)): %.idl: %
	$(VERBOSE)PWD=$(PWD)/$(subst .idl,,$@) $(MAKE) -C $(subst .idl,,$@) idl

$(addsuffix .lib,$(SUBDIRS)): %.lib: %
	$(VERBOSE)PWD=$(PWD)/$(subst .lib,,$@) $(MAKE) -C $(subst .lib,,$@) lib

# these targets are phony
.PHONY: include idl lib
