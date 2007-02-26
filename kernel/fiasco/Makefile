#
# $Id$
#
# GLOBAL Makefile for FIASCO

# enviroment
DFLBUILDDIR	:= build
ALLBUILDDIR	:= build-all
TEMPLDIR	:= src/templates
MAKEFILETEMPL	:= $(TEMPLDIR)/Makefile.builddir.templ
MANSUBDIRS	:= man
INSTALLSUBDIRS	:= $(MANSUBDIRS)
CLEANSUBDIRS	:= $(MANSUBDIRS) $(wildcard $(DFLBUILDDIR))
CONFIG_FILE	:= $(TEMPLDIR)/globalconfig.out
TEST_TEMPLATES	:= $(patsubst $(CONFIG_FILE).%,%,$(wildcard $(CONFIG_FILE).*))
DFL_TEMPLATE	:= ia32-short

getdir 		= $(shell						\
		    bd="$(1)";						\
		    if [ "$${bd\#/}" = "$$bd" -a			\
		         "$${bd\#*..}" = "$$bd" ]; then			\
		      relp="..";					\
		      while [ "$${bd\#*/}" != "$$bd" ]; do		\
		        relp="$$relp/..";				\
			bd="$${bd\#*/}";				\
		      done;						\
		      echo "$$relp";					\
		    else						\
		      pwd;						\
		    fi							\
		  )

buildmakefile = mkdir -p "$(1)";					     \
		perl -p -i -e '$$s = "$(CURDIR)/src"; s/\@SRCDIR\@/$$s/' \
			< $(MAKEFILETEMPL) > $(1)/Makefile

ifneq ($(strip $(B)),)
BUILDDIR := $(B)
endif
ifneq ($(strip $(BUILDDIR)),)
builddir:
		@echo "Creating build directory \"$(BUILDDIR)\"..."
		@if [ -e "$(BUILDDIR)" ]; then			\
			echo "Already exists, aborting.";	\
			exit 1;					\
		fi
		@$(call buildmakefile,$(BUILDDIR))
		@echo "done."
endif

ifneq ($(strip $(T)),)
this:
		set -e;							      \
		test -f $(TEMPLDIR)/globalconfig.out.$(T);		      \
		bdir=T-$(DFLBUILDDIR)-$(T);				      \
		rm -rf $$bdir;						      \
		$(call buildmakefile,T-$(DFLBUILDDIR)-$(T));		      \
		cp $(TEMPLDIR)/globalconfig.out.$(T) $$bdir/globalconfig.out; \
		$(MAKE) -C $$bdir
endif

$(DFLBUILDDIR): fiasco.builddir.create
		$(MAKE) -C $@

all:		fiasco man

clean cleanall:
		set -e; for i in $(CLEANSUBDIRS); do $(MAKE) -C $$i $@; done

purge: cleanall
		$(RM) -r $(DFLBUILDDIR)

man:
		set -e; for i in $(MANSUBDIRS); do $(MAKE) -C $$i; done

fiasco.builddir.create:
		[ -e $(DFLBUILDDIR)/Makefile ] ||			 \
			($(call buildmakefile,$(DFLBUILDDIR)))
		[ -f $(DFLBUILDDIR)/globalconfig.out ] || {		 \
			cp  $(TEMPLDIR)/globalconfig.out.$(DFL_TEMPLATE) \
				$(DFLBUILDDIR)/globalconfig.out;	 \
		}

config xconfig menuconfig oldconfig: fiasco.builddir.create
		$(MAKE) -C $(DFLBUILDDIR) $@

fiasco: fiasco.builddir.create
		$(MAKE) -C $(DFLBUILDDIR)

checkall l4check:
		error=0;						      \
		$(RM) -r $(ALLBUILDDIR);				      \
		for X in $(TEST_TEMPLATES); do				      \
			echo -e "\n= Building configuration: $$X\n\n";	      \
			$(call buildmakefile,$(ALLBUILDDIR)/$$X);	      \
			cp $(TEMPLDIR)/globalconfig.out.$$X		      \
			   $(ALLBUILDDIR)/$$X/globalconfig.out;		      \
			if $(MAKE) -C $(ALLBUILDDIR)/$$X; then		      \
				[ -z "$(KEEP_BUILD_DIRS)" ] &&		      \
				   $(RM) -r $(ALLBUILDDIR)/$$X;		      \
			else						      \
				error=$$?;				      \
				failed="$$failed $$X";			      \
			fi						      \
		done;							      \
		rmdir $(ALLBUILDDIR) >/dev/null 2>&1;			      \
		[ "$$failed" ] && echo -e "\nFailed configurations:$$failed"; \
		exit $$error;


help:
		@echo
		@echo "fiasco                    Builds the default configuration ($(DFL_TEMPLATE))"
		@echo "T=template                Build a certain configuration"
		@echo "checkall                  Build all template configurations in one go"
		@echo
		@echo "config menuconfig xconfig oldconfig"
		@echo "                          Configure kernel in \"$(DFLBUILDDIR)\""
		@echo "$(DFLBUILDDIR)                     Build kernel in \"$(DFLBUILDDIR)\""
		@echo "clean cleanall            clean or cleanall in \"$(CLEANSUBDIRS)\""
		@echo "purge                     cleanall, remove \"$(DFLBUILDDIR)\" and build helper"
		@echo
		@echo "Creating a custom kernel:"
		@echo
		@echo "  Create a build directory with:"
		@echo "     make BUILDDIR=builddir"
		@echo "  Then build the kernel:"
		@echo "     cd builddir"
		@echo "     make config"
		@echo "     make"
		@echo
		@echo "Call \"make help\" in the build directory for more information on build targets."
		@echo
		@echo "Default target: $(DFLBUILDDIR)"
		@echo

.PHONY:		man install clean cleanall fiasco.builddir.create fiasco \
		l4check checkall help config oldconfig menuconfig xconfig
