# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Template for include directories
#
# $Id$
#
# $Author$

#
# supported targets:
#
#   all				- the default, link the includes into the
#				  local include dir
#   install			- install the includes into the global
#				  include dir
#   config			- do nothing, may be overwritten
#   relink			- reinstall locally


INSTALLDIR_INC		?= $(DROPS_STDDIR)/include
INSTALLDIR_INC_LOCAL	?= $(L4DIR)/include
INSTALLFILE_INC		?= $(INSTALL) -m 644 $(1) $(2)
INSTALLFILE_INC_LOCAL	?= $(LN) -sf $(1) $(2)

INSTALLFILE		= $(INSTALLFILE_INC)
INSTALLDIR		= $(INSTALLDIR_INC)
INSTALLFILE_LOCAL	= $(INSTALLFILE_INC_LOCAL)
INSTALLDIR_LOCAL	= $(INSTALLDIR_INC_LOCAL)

INSTALL_TARGET		= $(shell find . -name \*.h -printf "%P ")
ifneq ($(origin INSTALL_INC_PREFIX), undefined)
$(warning You have overwritten INSTALL_INC_PREFIX. I hope you know what you are doing.)
endif
INSTALL_INC_PREFIX	?= l4/$(PKGNAME)

include $(L4DIR)/mk/Makeconf
.general.d: $(L4DIR)/mk/include.mk
-include $(DEPSVAR)
ifeq ($(filter scrub clean cleanall help,$(MAKECMDGOALS)),)
-include Makefile.inc
endif

all:: $(INSTALL_TARGET_LOCAL)

Makefile.inc:
	$(BUILD_MESSAGE)
	@echo '$@: .general.d' >$@
	@echo 'currentdothfile = $(INSTALL_TARGET)' >>$@
	@echo 'ifneq ($$(strip $$(currentdothfile)),$$(strip $$(INSTALL_TARGET)))' >>$@
	@echo 'Makefile.inc: FORCE' >>$@
	@echo 'endif' >>$@
	@echo $(INSTALL_TARGET) |  perl -e ' $$dir="$(shell pwd)";\
		while(<>){split; while(@_){$$_=shift @_; \
		  $$src=$$_; \
		  if( s#^ARCH-([^/]*)/L4API-([^/]*)/([^ ]*\.h)$$#\1/\2/$(INSTALL_INC_PREFIX)/\3# || \
		    s#^ARCH-([^/]*)/([^ ]*\.h)$$#\1/$(INSTALL_INC_PREFIX)/\2# || \
		    s#^([^ ]*\.h)$$#$(INSTALL_INC_PREFIX)/\1#){ \
		    print("$(L4DIR)/include/$$_ $(DROPS_STDDIR)/include/$$_:" \
			  . " $$dir/$$src\n"); \
		    $$inst_local .= "$(L4DIR)/include/$$_ "; \
		    $$inst_global .= "$(DROPS_STDDIR)/include/$$_ " \
		  }}} \
		print "INSTALL_TARGET_LOCAL = $$inst_local\n"; \
		print "INSTALL_TARGET_GLOBAL= $$inst_global\n"; ' >>$@

$(INSTALL_TARGET_LOCAL):
	$(INSTALL_LOCAL_MESSAGE)
	$(VERBOSE)$(INSTALL) -d $(dir $@)
	$(VERBOSE)$(call INSTALLFILE_LOCAL,$<,$@)

$(INSTALL_TARGET_GLOBAL):
	$(INSTALL_MESSAGE)
	$(VERBOSE)$(INSTALL) -d $(dir $@)
	$(VERBOSE)$(call INSTALLFILE,$<,$@)

install:: $(INSTALL_TARGET_GLOBAL)

# unconditionally install on "make install"
.PHONY: $(INSTALL_TARGET_GLOBAL)

cleanall::
	$(VERBOSE)$(RM) Makefile.inc

help::
	@echo "  all            - install files to $(INSTALLDIR_LOCAL)"
	@echo "  install        - install files to $(INSTALLDIR)"
	@echo "  scrub          - delete backup and temporary files"
	@echo "  clean          - same as scrub"
	@echo "  cleanall       - same as scrub"
	@echo "  help           - this help"

scrub clean cleanall::
	$(VERBOSE)$(SCRUB)
