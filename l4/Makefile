#
# $Id$
#
# GLOBAL Makefile for the whole L4 tree
#

L4DIR		?= .

BUILD_DIRS	= tool ../kernel/fiasco ../dice pkg
install-dirs	= tool ../kernel/fiasco ../dice pkg
clean-dirs	= tool ../kernel/fiasco ../dice pkg doc
cleanall-dirs	= tool ../kernel/fiasco ../dice pkg doc

BUILD_TOOLS	= gawk

# our default target is all::
all::

#####################
# config-tool

DROPSCONF 		= y
DROPSCONF_DEFCONFIG	?= $(L4DIR)/mk/defconfig/config.x86
DROPSCONF_CONFIG_IN	= $(L4DIR)/mk/config.in
DROPSCONF_CONFIG	= $(OBJ_BASE)/.Makeconf.bid
DROPSCONF_CONFIG_H	= $(OBJ_BASE)/include/l4/bid_config.h
DROPSCONF_CONFIG_MK	= $(OBJ_BASE)/.Makeconf.bid.converted
DROPSCONF_DONTINC_MK	= y
DROPSCONF_HELPFILE	= $(L4DIR)/mk/config.help

# separation in "dependent" (ie opts the build output depends on) and
# "independent" (ie opts the build output does not depend on) opts
CONFIG_MK_INDEPOPTS	= BID_GENERATE_MAPFILE\
			  DEPEND_VERBOSE_SWITCH DEPEND_VERBOSE VERBOSE_SWITCH \
			  SHOWMESSAGES BID_COLORED_PHASES HAVE_LDSO \
			  INT_CPP_NAME_SWITCH BID_LIBGENDEP INT_CPP_.*_NAME \
			  INT_CXX_.*_NAME VERBOSE BID_STRIP_PROGS \
			  INT_LD_NAME_SWITCH INT_LD_.*_NAME
CONFIG_MK_REAL		= $(OBJ_BASE)/Makeconf.bid.local
CONFIG_MK_INDEP		= $(OBJ_BASE)/Makeconf.bid.local.indep

ifneq ($(filter help config txtconfig oldconfig,$(MAKECMDGOALS)),)
# tweek $(L4DIR)/mk/Makeconf to use the intermediate file
export BID_IGN_ROOT_CONF=y
BID_ROOT_CONF=$(OBJ_BASE)/.Makeconf.bid.converted
endif

# $(L4DIR)/mk/Makeconf shouln't include Makeconf.local twice
MAKECONFLOCAL		= /dev/null
include $(L4DIR)/mk/Makeconf
export DROPS_STDDIR

# after having absfilename, we can export BID_ROOT_CONF
ifneq ($(filter config txtconfig oldconfig, $(MAKECMDGOALS)),)
export BID_ROOT_CONF=$(call absfilename,$(OBJ_BASE))/.Makeconf.bid.converted
endif

#####################
# rules follow

# some dependencies
all::	$(BUILD_DIRS)
tool:	$(DROPSCONF_CONFIG_MK)
pkg:	$(DROPSCONF_CONFIG_MK) tool
doc:	pkgdoc

ifeq ($(BID_BUILD_DOC),y)
install-dirs += doc
all:: doc
endif

tool ../kernel/fiasco pkg: ../dice
	$(VERBOSE)if [ -r $@/Makefile ]; then PWD=$(PWD)/$@ $(MAKE) -C $@; fi

../dice:
	$(VERBOSE)if [ -r $@/Makefile.drops ]; then                \
	   $(MAKE) -C $@ -f Makefile.drops;                        \
	 fi

doc:
	$(VERBOSE)for d in tool doc ; do \
		test ! -r $$d/Makefile || $(MAKE) -C $$d $@ ; done

pkgdoc:
	$(VERBOSE)test ! -r pkg/Makefile || PWD=$(PWD)/pkg $(MAKE) -C pkg doc


.PHONY: all clean cleanall install hello pkgdoc
.PHONY: $(BUILD_DIRS) doc/html check_build_tools

cleanall::
	$(VERBOSE)rm -f *~

clean cleanall install::
	$(VERBOSE)set -e; for i in $($@-dirs) ; do \
	  if [ -r $$i/Makefile -o -r $$i/GNUmakefile ] ; then \
		PWD=$(PWD)/$$i $(MAKE) -C $$i $@ ; fi ; done

install::
	$(VERBOSE)tmpdir=$$(mktemp -d -t l4bid.defs.XXXXXX) &&       \
	echo "L4DIR = $(L4DIR_ABS)"       > $$tmpdir/Makefile &&     \
	echo "L4_BUILDDIR = $(OBJ_BASE)" >> $$tmpdir/Makefile &&     \
	cat $(L4DIR)/mk/export_defs.inc  >> $$tmpdir/Makefile &&     \
	$(MAKE) -C $$tmpdir;                                         \
	$(RM) -r $$tmpdir


#####################
# config-rules follow

# it becomes a bit confusing now: 'make config' results in a config file, which
# must be postprocessed. This is done in config.inc. Then,
# we evaluate some variables that depend on the postprocessed config file.
# The variables are defined in mk/Makeconf, which sources Makeconf.bid.local.
# Hence, we have to 1) postprocess, 2) call make again to get the variables.
DROPSCONF_CONFIG_MK_POST_HOOK:: check_build_tools $(OBJ_DIR)/Makefile
        # libgendep must be done before calling make with the local helper
	$(VERBOSE)$(MAKE) libgendep
	$(VERBOSE)$(MAKE) Makeconf.bid.local-helper || \
		(rm -f $(DROPSCONF_CONFIG_MK) $(CONFIG_MK_REAL) $(CONFIG_MK_INDEP); false)
	$(VEROBSE)$(LN) -snf $(L4DIR_ABS) $(OBJ_BASE)/source

# cashing of some variables. Others are determined directly.
# The contents of the variables to cache is already defined in mk/Makeconf.
.PHONY: Makeconf.bid.local-helper libgendep
ARCH = $(BUILD_ARCH)
Makeconf.bid.local-helper:
	$(VERBOSE)echo BUILD_SYSTEMS="$(strip $(ARCH)_$(CPU)            \
	               $(ARCH)_$(CPU)-$(BUILD_ABI))" >> $(DROPSCONF_CONFIG_MK)
	$(VERBOSE)$(foreach v, GCCDIR GCCLIB GCCLIB_EH GCCVERSION	\
			GCCMAJORVERSION GCCMINORVERSION GCCSUBVERSION	\
			LDNOSTDLIB GCCLDNOSTDLIB,			\
			echo $(v)=$(call $(v)_f,$(ARCH))	\
			>>$(DROPSCONF_CONFIG_MK);)
	$(foreach v, BID_LIBGENDEP, echo $v=$($(v)) >>$(DROPSCONF_CONFIG_MK);)
	$(VERBOSE)echo 'BUILD_PIC += $$(BUILD_LOADER_PICS)' >>$(DROPSCONF_CONFIG_MK)
ifeq ($(INT_CPP_NAME_SWITCH), y)
	$(VERBOSE) set -e; X="tmp.$$$$$$RANDOM.c" ; echo 'int main(void){}'>$$X ; \
		rm -f $$X.out ; LD_PRELOAD=$(BID_LIBGENDEP) GENDEP_SOURCE=$$X \
		GENDEP_OUTPUT=$$X.out $(CC) -c $$X -o $$X.o; \
		test -e $$X.out; echo INT_CPP_NAME=`cat $$X.out` \
			>>$(DROPSCONF_CONFIG_MK); \
		rm -f $$X $$X.{o,out};
	$(VERBOSE)set -e; X="tmp.$$$$$$RANDOM.cc" ; echo 'int main(void){}'>$$X; \
		rm -f $$X.out; LD_PRELOAD=$(BID_LIBGENDEP) GENDEP_SOURCE=$$X \
		GENDEP_OUTPUT=$$X.out $(CXX) -c $$X -o $$X.o; \
		test -e $$X.out; echo INT_CXX_NAME=`cat $$X.out` \
			>>$(DROPSCONF_CONFIG_MK); \
		rm -f $$X $$X.{o,out};
endif
ifeq ($(INT_LD_NAME_SWITCH), y)
	$(VERBOSE) set -e; X="tmp.$$$$$$RANDOM.c" ; echo 'int main(void){}'>$$X ; \
		rm -f $$X.out ; LD_PRELOAD=$(BID_LIBGENDEP) GENDEP_SOURCE=$$X. \
		GENDEP_OUTPUT=$$X.out $(CC) $$X -o $$X.; \
		test -e $$X.out; echo INT_LD_NAME=`cat $$X.out` \
			>>$(DROPSCONF_CONFIG_MK); \
		rm -f $$X $$X.{,out};
endif
	$(VERBOSE)sort <$(DROPSCONF_CONFIG_MK) >$(CONFIG_MK_REAL).tmp
	$(VERBOSE)echo -e "# Automatically generated. Don't edit\n" >$(CONFIG_MK_INDEP)
	$(VERBOSE)grep $(addprefix -e ^,$(CONFIG_MK_INDEPOPTS) ) \
		<$(CONFIG_MK_REAL).tmp >>$(CONFIG_MK_INDEP)
	$(VERBOSE)echo -e "# Automatically generated. Don't edit\n" >$(CONFIG_MK_REAL).tmp2
	$(VERBOSE)grep -v $(addprefix -e ^,$$ # $(CONFIG_MK_INDEPOPTS) ) \
		<$(CONFIG_MK_REAL).tmp >>$(CONFIG_MK_REAL).tmp2
	$(VERBOSE)echo -e 'include $(call absfilename,$(CONFIG_MK_INDEP))' >>$(CONFIG_MK_REAL).tmp2
	$(VERBOSE)if [ -e "$(CONFIG_MK_REAL)" ]; then                        \
	            diff --brief $(CONFIG_MK_REAL) $(CONFIG_MK_REAL).tmp2 || \
		      mv $(CONFIG_MK_REAL).tmp2 $(CONFIG_MK_REAL);           \
		  else                                                       \
		    mv $(CONFIG_MK_REAL).tmp2 $(CONFIG_MK_REAL);             \
		  fi
	$(VERBOSE)$(RM) $(CONFIG_MK_REAL).tmp $(CONFIG_MK_REAL).tmp2

libgendep:
	$(VERBOSE)if [ ! -r tool/gendep/Makefile ]; then	\
	            echo "=== l4/tool/gendep missing! ===";	\
		    exit 1;					\
	          fi
	$(VERBOSE)PWD=$(PWD)/tool/gendep $(MAKE) -C tool/gendep

check_build_tools:
	@unset mis;                                                \
	for i in $(BUILD_TOOLS); do                                \
	  if ! command -v $$i >/dev/null 2>&1; then                \
	    [ -n "$$mis" ] && mis="$$mis ";                        \
	    mis="$$mis$$i";                                        \
	  fi                                                       \
	done;                                                      \
	if [ -n "$$mis" ]; then                                    \
	  echo -e "\033[1;31mProgram(s) \"$$mis\" not found, please install!\033[0m"; \
	  exit 1;                                                  \
	fi
