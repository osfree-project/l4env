PKGDIR	= .
L4DIR	?= $(PKGDIR)/../..


TARGET  = include lib
SUBDIRS = uart

include $(L4DIR)/mk/subdir.mk

clean:: $(addsuffix .clean,$(SUBDIRS))

cleanall:: $(addsuffix .cleanall,$(SUBDIRS))

include: uart.include

lib: uart.lib


# explict dependencies

uart: uart.lib

%.clean: 
	$(MAKE) -C $* clean

%.cleanall:
	$(MAKE) -C $* cleanall

$(addsuffix .include,$(SUBDIRS)): %.include: %
	$(MAKE) -C $(subst .include,,$@) include

$(addsuffix .idl,$(SUBDIRS)): %.idl: %
	$(MAKE) -C $(subst .idl,,$@) idl

$(addsuffix .lib,$(SUBDIRS)): %.lib: %
	$(MAKE) -C $(subst .lib,,$@) lib


# these targets are phony
.PHONY: include idl lib
