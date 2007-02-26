#
# $Id$
#
# GLOBAL Makefile for FIASCO

# enviroment
DFLBUILDDIR	:= build
TEMPLDIR	:= src/templates
MAKEFILETEMPL	:= $(TEMPLDIR)/Makefile.builddir.templ
MANSUBDIRS	:= man
INSTALLSUBDIRS	:= $(MANSUBDIRS)
CLEANSUBDIRS	:= $(MANSUBDIRS) $(DFLBUILDDIR)
CONFIG_FILE	:= $(TEMPLDIR)/globalconfig.h
TEST_TEMPLATES	:= $(patsubst $(CONFIG_FILE).%,%,$(wildcard $(CONFIG_FILE).*))
DFL_TEMPLATE	:= ia32-short

ifneq ($(strip $(BUILDDIR)),)
create-build-dir:
		@echo "Creating build directory \"$(BUILDDIR)\"..."
		@if [ -e "$(BUILDDIR)" ]; then			\
			echo "Already exists, aborting.";	\
			exit 1;					\
		fi
		@mkdir -p "$(BUILDDIR)"
		@perl -p -i -e '$$s = "$(CURDIR)/src"; s/^srcdir.+/srcdir := \"$$s\"/' < $(MAKEFILETEMPL) > "$(BUILDDIR)/Makefile"
		@echo "done."
endif

all:		fiasco man

clean cleanall:
		set -e; for i in $(CLEANSUBDIRS); do $(MAKE) -C $$i $@; done

man:
		set -e; for i in $(MANSUBDIRS); do $(MAKE) -C $$i; done

fiasco.builddir.create:
		mkdir -p $(DFLBUILDDIR)
		[ -f $(DFLBUILDDIR)/Makefile ] ||					\
			cp $(TEMPLDIR)/Makefile.builddir.templ $(DFLBUILDDIR)/Makefile
		[ -f $(DFLBUILDDIR)/globalconfig.h ] || {				\
			cp  $(TEMPLDIR)/globalconfig.h.$(DFL_TEMPLATE)		\
				$(DFLBUILDDIR)/globalconfig.h;			\
			cp  $(TEMPLDIR)/globalconfig.out.$(DFL_TEMPLATE)	\
				$(DFLBUILDDIR)/globalconfig.out;			\
		}

config xconfig menuconfig: fiasco.builddir.create
		$(MAKE) -C $(DFLBUILDDIR) $@

fiasco: fiasco.builddir.create
		$(MAKE) -C $(DFLBUILDDIR)

l4check:
		error=0;							\
		for X in $(TEST_TEMPLATES) ; do					\
			rm -rf $(DFLBUILDDIR);					\
			mkdir -p $(DFLBUILDDIR);					\
			echo -e "\n= Building configuration: $$X\n\n";		\
			cp $(TEMPLDIR)/globalconfig.h.$$X   $(DFLBUILDDIR)/globalconfig.h;		\
			cp $(TEMPLDIR)/globalconfig.out.$$X $(DFLBUILDDIR)/globalconfig.out;	\
			cp $(TEMPLDIR)/Makefile.builddir.templ $(DFLBUILDDIR)/Makefile;		\
			$(MAKE) -C $(DFLBUILDDIR) || {				\
				error=$$?;					\
				failed="$$failed $$X";				\
			}							\
		done;								\
		rm -rf $(DFLBUILDDIR);						\
		[ "$$failed" ] && echo -e "\nFailed configurations:$$failed";	\
		exit $$error;

.PHONY:		man install clean cleanall fiasco.builddir.create fiasco l4check config
