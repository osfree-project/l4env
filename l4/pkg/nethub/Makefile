PKGDIR	= .
L4DIR	?= $(PKGDIR)/../..


TARGET  = include idl lib
SUBDIRS = server client_lib base_lib examples

include $(L4DIR)/mk/subdir.mk


idl: base_lib.idl

include: base_lib.include client_lib.include

lib: base_lib.lib client_lib.lib


# explict dependencies
server: base_lib.lib

client_lib: client_lib.lib

examples: client_lib.lib

$(addsuffix .include,$(SUBDIRS)):: %.include: %
	$(MAKE) -C $(subst .include,,$@) include

$(addsuffix .idl,$(SUBDIRS)):: %.idl: %
	$(MAKE) -C $(subst .idl,,$@) idl

$(addsuffix .lib,$(SUBDIRS)):: %.lib: %
	$(MAKE) -C $(subst .lib,,$@) lib

client_lib.lib:: base_lib.include base_lib.idl base_lib.lib

client_lib.include:: base_lib.include base_lib.idl


# these targets are phony
.PHONY: include idl lib
