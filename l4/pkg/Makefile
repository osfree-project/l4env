# $Id$
# GLOBAL Makefile for all packages

L4DIR ?= ..
include $(L4DIR)/mk/Makeconf

# Dreadful hack, but libstdc++ installs its includes on lib, not on include.
# Here we break the general rule: bin -> lib -> include. Never do a thing
# like this unless you really know what you are doing, as opposed to me.
libstdc++.headers: libstdc++.lib

# dm_phys uses IDLs of dm_mem
dm_phys.headers: dm_mem.headers
# dm_mem uses IDLs of dm_generic
dm_mem.headers: dm_generic.headers
# bootmod uses IDLs of dm_generic
bootmod.headers: dm_generic.headers
# IDL of qos_rmgr uses headers of qos_mgr
qos_rmgr.headers: qos_mgr.headers

# The directories/packages of l4env
L4ENV_SUBDIRS = \
	bootmod \
	buffermgr \
	crtx \
	dm_generic \
	dm_mem \
	dm_phys \
	generic_blk \
	generic_fprov \
	generic_io \
	generic_ts \
	l4env \
	l4io \
	l4rm \
	l4sys \
	l4util \
        lock \
	log \
	names \
	pci \
	omega0 \
	oskit_support \
	oskit_support_l4env \
	oskit10_support \
	oskit10_support_l4env \
	oskit10_support_l4env_full \
	oskit10_support_tiny \
	rmgr \
        semaphore \
	sigma0 \
	simple_ts \
	slab \
	tftp \
	thread \

L4CONF_FILES = 				\
	Makeconf			\
	Makeconf.flick			\
	Makeconf.lib.rules		\
	Makeconf.lib.targets		\
	Makeconf.oskit10_l4env		\
	Makeconf.oskit10_l4env_tiny	\
	Makeconf.stdtargets		\
	Makeconf.transitional		\

BID_STATE_FILE = BUILD.state

# all our packages
ALL_SUBDIRS	= $(shell find . -maxdepth 1 -type d ! -name CVS -printf %P\ )
# the broken packages
BROKEN_SUBDIRS	= $(patsubst %/broken, %, \
			$(wildcard $(addsuffix /broken,$(ALL_SUBDIRS))))
# the obsolete packages
OBSOLETE_SUBDIRS = $(patsubst %/obsolete, %, \
			$(wildcard $(addsuffix /obsolete,$(ALL_SUBDIRS))))
# and the packages we are supposed to build
BUILD_SUBDIRS	= $(filter-out $(BROKEN_SUBDIRS) $(OBSOLETE_SUBDIRS), \
			$(ALL_SUBDIRS))
# l4env dirs that are actually checked out and are to be built
L4ENV_BUILD_SUBDIRS = $(filter $(L4ENV_SUBDIRS), $(BUILD_SUBDIRS))

# the makefiles in the subdirs/packages
PKG_MAKEFILE_SYMLINKS=$(addsuffix /Makefile,$(BUILD_SUBDIRS) $(OBSOLETE_SUBDIRS))

#####################
# default make action is "all" - build all packages
#
# We build all l4env headers (idl+includes), then all l4env libs, then the
# l4env servers and examples. Then we build the other headers
# (idl+includes), the other libs, the other servers and examples. We use
# dependencies for this, witch might allow parallel build processes.
#
all::		bin
bin:		lib
lib:		headers
headers:	l4env
l4env:		l4env-lib
l4env-lib:	l4env-headers

bin:		$(addsuffix .bin,$(filter-out $(L4ENV_SUBDIRS),$(BUILD_SUBDIRS)))
lib:		$(addsuffix .lib,$(filter-out $(L4ENV_SUBDIRS),$(BUILD_SUBDIRS)))
headers:	$(addsuffix .headers,$(filter-out $(L4ENV_SUBDIRS),$(BUILD_SUBDIRS)))
l4env:		$(addsuffix .bin,$(L4ENV_BUILD_SUBDIRS))
l4env-lib:	$(addsuffix .lib,$(L4ENV_BUILD_SUBDIRS))
l4env-headers:	$(addsuffix .headers,$(L4ENV_BUILD_SUBDIRS))

#####################
# cont handling
ifeq ($(filter cont,$(MAKECMDGOALS)),)
BID_reset = reset
else
-include $(BID_STATE_FILE)
endif
reset:
	$(VERBOSE)$(RM) $(BID_STATE_FILE)
cont:
	$(VERBOSE)$(MAKE) $(addprefix -o ,reset $(BID_STATE_DONE))
BID_SAFE_STATE= @echo 'BID_STATE_DONE+=$@'>>$(BID_STATE_FILE)

.PHONY: all reset cont bin lib headers idl include
.PHONY: l4env l4env-lib l4env-headers l4env-include l4env-idl

ifeq "$(SHOWMESSAGES)" "true"
BIN_MESSAGE=@echo "=== Building servers, examples, doc of \"$(basename $@)\" ==="
LIB_MESSAGE=@echo "=== Building lib of \"$(basename $@)\" ==="
HDR_MESSAGE=@echo "=== Building IDL & includes of \"$(basename $@)\" ==="
INST_MESSAGE=echo "=== Installing Package \"$(1)\" ==="
TEMPLATE_MESSAGE=@echo "=== Creating symlinks to Makefile template in $(@D) ==="
endif

$(addsuffix .bin,$(BUILD_SUBDIRS)):%.bin:%/Makefile $(BID_reset)
	$(BIN_MESSAGE)	
	$(VERBOSE)$(MAKE) -C $(basename $@) all -o lib -o include -o idl
	$(VERBOSE)$(BID_SAFE_STATE)

$(addsuffix .lib,$(BUILD_SUBDIRS)):%.lib:%/Makefile $(BID_reset)
	$(LIB_MESSAGE)
	$(VERBOSE)$(MAKE) -C $(basename $@) lib -o include -o idl
	$(VERBOSE)$(BID_SAFE_STATE)

$(addsuffix .headers,$(BUILD_SUBDIRS)):%.headers:%/Makefile $(BID_reset)
	$(HDR_MESSAGE)
	$(VERBOSE)$(MAKE) -C $(basename $@) idl
	$(VERBOSE)$(BID_SAFE_STATE)

install:: $(PKG_MAKEFILE_SYMLINKS)
	$(VERBOSE)set -e; for i in $(BUILD_SUBDIRS); do \
	  $(call INST_MESSAGE,$$$$i); \
	  $(MAKE) -C $$i $@; \
	done

.PHONY: $(foreach ext, .bin .lib .include .idl .headers,addsuffix $(ext), $(BUILD_SUBDIRS))

clean cleanall:: $(BID_reset)
clean cleanall::
	@for i in $(BUILD_SUBDIRS) $(OBSOLETE_SUBDIRS); do \
	  echo "=== Cleaning in package  \"$$i\" ==="; \
	  if [ -r $$i/Makefile ] ; then $(MAKE) -C $$i $@; fi ; \
	done

del-backup:
	@rm -i `find . -name "*~"` `find . -name "#*#"`

pkg-links: $(PKG_MAKEFILE_SYMLINKS)

$(PKG_MAKEFILE_SYMLINKS):
	$(TEMPLATE_MESSAGE)
	$(LN) -s ../pkg.Makefile.tmpl $@

rm-pkg-links:
	$(VERBOSE)$(RM) -i `find -mindepth 2 -maxdepth 2 -type l -name "Makefile"`

rm-pkg-links-force:
	$(VERBOSE)$(RM) `find -mindepth 2 -maxdepth 2 -type l -name "Makefile"`


# Update Makefile on l4env_update.
ifneq ($(filter l4env_update,$(MAKECMDGOALS)),)
.PHONY: Makefile
Makefile:
	cvs up $@
endif

l4env_update: l4conf_update gcc-wrap_update $(BID_reset)
	for i in $(L4ENV_SUBDIRS); do \
	  echo "=== Updating \"$$i\" ==="; \
	  cvs update -d $$i; \
	done
	cvs update pkg.Makefile.tmpl

gcc-wrap:
	$(VERBOSE)$(MAKE) -C $(L4DIR)/tool/gcc-wrap

gcc-wrap_update:
	$(VERBOSE)cd $(L4DIR) && cvs update -d tool/gcc-wrap

l4conf_update:
	set -e; \
	( cd ..; cvs update $(L4CONF_FILES) )

relocate:
	@for i in $(BUILD_SUBDIRS); do 					\
	     if [ -f $$i/server/src/Makefile ]; then			\
	       echo "=== Building \"$$i\" relocatable ===";		\
	       $(MAKE) -C $$i/server/src reloc;				\
	     else			 				\
	       echo "( Package \"$$i\" has no server/src dir )"; 	\
	     fi								\
	 done;								\

relink:
	@for i in $(BUILD_SUBDIRS); do 					\
	     if [ -f $$i/server/src/Makefile ]; then			\
	       echo "=== Re-linking \"$$i\" server ===";		\
	       $(MAKE) -C $$i/server/src;				\
	     else			 				\
	       echo "( Package \"$$i\" has no server/src dir )"; 	\
	     fi;							\
	     if [ -f $$i/examples/Makefile ]; then			\
	       echo "=== Re-linking \"$$i\" examples ===";		\
	       $(MAKE) -C $$i/examples;					\
	     else							\
	       echo "( Package \"$$i\" has no examples dir )";		\
	     fi;							\
	 done;								\


print-subdirs:
	@echo $(BUILD_SUBDIRS)
	@echo obsolete: $(OBSOLETE_SUBDIRS)

help:
	@echo "Specify one of the following targets:"
	@echo "all             - build the l4env and the packages neither broken nor obsolete"
	@echo "cont            - after correcting errors, continue where \"make all\" failed"
	@echo "install         - install the packages, use only after making all!"
	@echo "clean           - clean the packages"
	@echo "cleanall        - clean the packages pedanticly"
	@echo "del-backup      - remove temporary file in this tree"
	@echo "pkg-links       - create symlinks to pkg.Makefile template in all subdirs"
	@echo "rm-pkg-links    - delete symlinked Makefiles in the subdirs interactively"
	@echo "rm-pkg-links-force - dito, non-interactively"
	@echo "headers         - short for includes + idl"
	@echo "idl             - generate IDLs in the subdirs and link them to the L4DIR"
	@echo "includes        - link the includes in the subdirs to the L4DIR"
	@echo "lib             - generate the libraries in the subdirs and link them"
	@echo "l4env-headers   - headers target for the L4ENV dirs"
	@echo "l4env-idl       - idl target for the L4ENV dirs"
	@echo "l4env-include   - include target for the L4ENV dirs"
	@echo "l4env-lib       - lib target for the L4ENV dirs"
	@echo "l4env           - build the l4env"
	@echo "l4env_update    - cvsupdate of the L4ENV dirs"
	@echo "l4conf_update   - cvsupdate the diverse Makeconf.* in the L4DIR"
	@echo "relocate        - building the relocatable binaries in the dirs"
	@echo "relink          - rebuild servers and examples"
	@echo "reset           - reset the state used by the \"cont\" target"
	@echo "print-subdirs   - print the SUBDIRS which are subject to build"
	@echo "help            - this help"
	@echo -e "\nWhen building, you can omit sub-targets using the \"-o <tgt>\" switch to make."
	@echo "\"make -o l4env\" skips rebuilding the l4env, \"make -o lib\" only builds"
	@echo "binaries and examples."

# global defines

.PHONY:		install clean cleanall kernels
