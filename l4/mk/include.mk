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

INSTALLDIR		= $(INSTALLDIR_INC)
INSTALLDIR_LOCAL	= $(INSTALLDIR_INC_LOCAL)

ifeq ($(origin TARGET),undefined)
TARGET_CMD		:= find . -name \*.h -printf "%P "
else
TARGET_CMD		:= echo $(TARGET)
endif
ifneq ($(origin INSTALL_INC_PREFIX), undefined)
$(warning You have overwritten INSTALL_INC_PREFIX. I hope you know what you are doing.)
endif
INSTALL_INC_PREFIX	?= l4/$(PKGNAME)

include $(L4DIR)/mk/Makeconf
.general.d: $(L4DIR)/mk/include.mk
-include $(DEPSVAR)


installscript = perl -e 'chomp($$dir=`pwd`); while(<>){split;while(@_){\
	$$_=shift @_; $$src=$$_; \
	if( s|^ARCH-([^/]*)/L4API-([^/]*)/([^ ]*)$$|\1/\2/$(INSTALL_INC_PREFIX)/\3| ||\
	s|^ARCH-([^/]*)/([^ ]*)$$|\1/$(INSTALL_INC_PREFIX)/\2| ||\
	s|^([^ ]*)$$|$(INSTALL_INC_PREFIX)/\1|){\
	$$src="$$dir/$$src";\
	$$dstdir=$$dst="$(if $(1),$(INSTALLDIR_LOCAL),$(INSTALLDIR))/$$_";\
	$$dstdir=~s|/[^/]*$$||; \
	-d $$dstdir || system("install","-vd",$$dstdir) && exit 1; \
	$(if $(1), \
	  readlink($$dst) eq $$src||system("ln","-sfv",$$src,$$dst)&&exit 1;,\
	  system("install","-vm","644",$$src,$$dst) && exit 1;) }}}'

all:: 
	@$(TARGET_CMD) | $(call installscript,1)

install::
	@$(TARGET_CMD) | $(call installscript);

cleanall::
	$(VERBOSE)$(RM) .general.d

help::
	@echo "  all            - install files to $(INSTALLDIR_LOCAL)"
	@echo "  install        - install files to $(INSTALLDIR)"
	@echo "  scrub          - delete backup and temporary files"
	@echo "  clean          - same as scrub"
	@echo "  cleanall       - same as scrub"
	@echo "  help           - this help"

scrub clean cleanall::
	$(VERBOSE)$(SCRUB)
