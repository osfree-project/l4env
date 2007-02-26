#
# $Id$
#
# GLOBAL Makefile for the whole L4 tree
#

L4DIR		?= .

OSKITDIR	?= ../oskit
OSKIT10DIR	?= ../oskit10

BUILD_DIRS	= tool $(OSKITDIR) $(OSKIT10DIR) $(L4LINUX22_DIR) kernel pkg
HELLODIRS	= pkg/l4sys \
		  pkg/crtx \
		  pkg/rmgr \
		  pkg/l4util \
		  pkg/oskit_support \
		  pkg/sigma0 \
		  pkg/hello
install-dirs	= tool $(OSKITDIR) $(OSKIT10DIR) kernel pkg doc
clean-dirs	= tool $(OSKITDIR) $(OSKIT10DIR) $(L4LINUX22_DIR) kernel pkg doc
cleanall-dirs	= tool kernel pkg doc

# our default target is all::
all::

#####################
# config-tool

DROPSCONF 		= y
DROPSCONF_DEFCONFIG	= $(L4DIR)/mk/defconfig/config.x86
DROPSCONF_CONFIG_IN	= $(L4DIR)/mk/config.in
DROPSCONF_CONFIG	= $(L4DIR)/.Makeconf.bid
DROPSCONF_CONFIG_H	= $(L4DIR)/include/l4/bid_config.h
DROPSCONF_CONFIG_MK	= $(L4DIR)/.Makeconf.bid.converted
DROPSCONF_DONTINC_MK	= y
DROPSCONF_HELPFILE	= $(L4DIR)/mk/config.help

# separation in "dependent" (ie opts the build output depends on) and
# "independent" (ie opts the build output does not depend on) opts
CONFIG_MK_INDEPOPTS	= DEPEND_VERBOSE_SWITCH DEPEND_VERBOSE VERBOSE_SWITCH \
			  SHOWMESSAGES BID_COLORED_PHASES HAVE_LDSO \
			  INT_CPP_NAME_SWITCH BID_LIBGENDEP INT_CPP_.*_NAME \
			  INT_CXX_.*_NAME VERBOSE BID_STRIP_PROGS
CONFIG_MK_REAL		= Makeconf.bid.local
CONFIG_MK_INDEP		= Makeconf.bid.local.indep

ifneq ($(filter help config txtconfig oldconfig,$(MAKECMDGOALS)),)
# tweek $(L4DIR)/mk/Makeconf to use the intermediate file
export BID_IGN_ROOT_CONF=y
BID_ROOT_CONF=$(L4DIR)/.Makeconf.bid.converted
endif

# $(L4DIR)/mk/Makeconf shouln't include Makeconf.local twice
MAKECONFLOCAL		= /dev/null
include $(L4DIR)/mk/Makeconf
export DROPS_STDDIR

# after having absfilename, we can export BID_ROOT_CONF
ifneq ($(filter config txtconfig oldconfig, $(MAKECMDGOALS)),)
export BID_ROOT_CONF=$(call absfilename,$(L4DIR))/.Makeconf.bid.converted
endif

#####################
# rules follow

# some dependencies
all::	$(BUILD_DIRS) doc
tool:	$(DROPSCONF_CONFIG_MK)
pkg:	$(DROPSCONF_CONFIG_MK) tool $(OSKITDIR) $(OSKIT10DIR) $(L4LINUX22_DIR)
doc:	pkgdoc
kernel: $(DROPSCONF_CONFIG_MK) tool

tool pkg kernel:
	$(VERBOSE)if [ -r $@/Makefile ]; then $(MAKE) -C $@; fi

$(OSKITDIR) $(OSKIT10DIR) $(L4LINUX22_DIR): $(DROPSCONF_CONFIG_MK)
	$(VERBOSE)if [ -r $@/Makefile.drops ]; then \
		$(MAKE) -C $@ -f Makefile.drops; fi

doc:
	$(VERBOSE)for d in tool doc ; do \
		test ! -r $$d/Makefile || $(MAKE) -C $$d $@ ; done

pkgdoc:
	$(VERBOSE)test ! -r pkg/Makefile || $(MAKE) -C pkg doc


# 'Hello World' Server is special: It requires oskit to be present
hello:	oskit.present $(OSKITDIR)
	$(VERBOSE)set -e; for i in $(HELLODIRS) ; do \
	  $(MAKE) -C $$i lib ; \
	done
	$(VERBOSE)set -e; for i in $(HELLODIRS) ; do \
	  test $$i = pkg/l4util || $(MAKE) -C $$i ; \
	done

oskit.present:
	@test -r $(OSKITDIR)/Makefile.drops || \
	  (echo "ERROR: please install OSKIT to $(OSKITDIR)"; exit 1)

.PHONY: all clean cleanall install hello oskit.present pkgdoc
.PHONY: $(BUILD_DIRS) $(HELLODIRS) doc/html

cleanall::
	$(VERBOSE)rm -f *~

clean cleanall install::
	$(VERBOSE)set -e; for i in $($@-dirs) ; do \
	  if [ -r $$i/Makefile -o -r $$i/GNUmakefile ] ; then \
		$(MAKE) -C $$i $@ ; fi ; done

#####################
# config-rules follow

# it becomes a bit confusing now: 'make config' results in a config file, which
# must be postprocessed. This is done in config.inc. Then,
# we evaluate some variables that depend on the postprocessed config file.
# The variables are defined in mk/Makeconf, which sources Makeconf.bid.local.
# Hence, we have to 1) postprocess, 2) call make again to get the variables.
DROPSCONF_CONFIG_MK_POST_HOOK::
        # try-libgendep must be done before calling make with the local helper
	$(VERBOSE)$(MAKE) try-libgendep
	$(VERBOSE)$(MAKE) Makeconf.bid.local-helper || \
		(rm -f $(DROPSCONF_CONFIG_MK) $(CONFIG_MK_REAL) $(CONFIG_MK_INDEP); false)

# cashing of some variables. Others are determined directly.
# The contents of the variables to cache is already defined in mk/Makeconf.
.PHONY: Makeconf.bid.local-helper try-libgendep
ARCHS = $(BUILD_SYSTEMS_x86) $(BUILD_SYSTEMS_arm)
Makeconf.bid.local-helper:
	$(foreach arch,$(ARCHS),$(if $(CPU_$(arch)),, \
	    $(error Specify at least one CPU-type for architecture "$(arch)")))
	$(VERBOSE)echo BUILD_SYSTEMS="$(strip				\
	    $(foreach arch, $(ARCHS), $(foreach cpu,$(CPU_$(arch)),	\
		$(foreach l4api, l4v2 l4x0 l4x2 l4v4,			\
		    $(if $(filter y,$(BUILD_SYSTEMS_$(arch)_$(l4api))),	\
			$(arch)_$(cpu)-$(l4api)))			\
		    $(arch)_$(cpu) )))" >>$(DROPSCONF_CONFIG_MK)
	$(VERBOSE)$(foreach arch, $(ARCHS), \
		$(foreach v, GCCDIR GCCLIB GCCLIB_EH GCCVERSION,\
			echo $(v)_$(arch)=$(call $(v)_f,$(arch)) \
			>>$(DROPSCONF_CONFIG_MK);))
	$(foreach v, BID_LIBGENDEP, echo $v=$($(v)) >>$(DROPSCONF_CONFIG_MK);)
	$(VERBOSE)echo 'BUILD_PIC += $$(BUILD_LOADER_PICS)' >>$(DROPSCONF_CONFIG_MK)
ifeq ($(INT_CPP_NAME_SWITCH), y)
	$(VERBOSE)$(foreach arch, $(ARCHS), \
		set -e; X=`tempfile -s .c` ; echo 'int main(void){}'>$$X ; \
		rm -f $$X.out ; LD_PRELOAD=$(BID_LIBGENDEP) GENDEP_SOURCE=$$X \
		GENDEP_OUTPUT=$$X.out $(CC_$(arch)) -c $$X -o $$X.o; \
		test -e $$X.out; echo INT_CPP_$(arch)_NAME=`cat $$X.out` \
			>>$(DROPSCONF_CONFIG_MK); )\
		rm -f $$X $$X.{c,out}
	$(VERBOSE)$(foreach arch, $(ARCHS), \
		set -e; X=`tempfile -s .cc` ; echo 'int main(void){}'>$$X; \
		rm -f $$X.out; LD_PRELOAD=$(BID_LIBGENDEP) GENDEP_SOURCE=$$X \
		GENDEP_OUTPUT=$$X.out $(CXX_$(arch)) -c $$X -o $$X.o; \
		test -e $$X.out; echo INT_CXX_$(arch)_NAME=`cat $$X.out` \
			>>$(DROPSCONF_CONFIG_MK) ; ) \
		rm -f $$X $$X.{cc,out}
endif
	$(VERBOSE)sort <$(DROPSCONF_CONFIG_MK) >$(CONFIG_MK_REAL).tmp
	$(VERBOSE)echo -e "# Automatically generated. Don't edit\n" >$(CONFIG_MK_INDEP)
	$(VERBOSE)grep $(addprefix -e ^,$(CONFIG_MK_INDEPOPTS) ) \
		<$(CONFIG_MK_REAL).tmp >>$(CONFIG_MK_INDEP)
	$(VERBOSE)echo -e "# Automatically generated. Don't edit\n" >$(CONFIG_MK_REAL).tmp2
	$(VERBOSE)grep -v $(addprefix -e ^,$$ # $(CONFIG_MK_INDEPOPTS) ) \
		<$(CONFIG_MK_REAL).tmp >>$(CONFIG_MK_REAL).tmp2
	$(VERBOSE)echo -e 'include $(call absfilename,$(CONFIG_MK_INDEP))' >>$(CONFIG_MK_REAL).tmp2
	$(VERBOSE)diff --brief $(CONFIG_MK_REAL) $(CONFIG_MK_REAL).tmp2 || \
		mv $(CONFIG_MK_REAL).tmp2 $(CONFIG_MK_REAL)
	$(VERBOSE)$(RM) $(CONFIG_MK_REAL).tmp $(CONFIG_MK_REAL).tmp2

# If a local libgendep is here, use it. Otherwise, the build process will
# utilize the global one; or fail if this is not available too.
try-libgendep:
	$(VERBOSE)test ! -r tool/gendep/Makefile || $(MAKE) -C tool/gendep
