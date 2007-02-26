# $Id$
# GLOBAL Makefile for all packages

L4DIR ?= ..
include $(L4DIR)/mk/Makeconf

# Dreadful hack, but libstdc++ installs its includes on lib, not on include.
# Here we break the general rule: bin -> lib -> include. Never do a thing
# like this unless you really know what you are doing, as opposed to me.
libstdc++.headers: libstdc++.lib

# other (mostly IDL-related) dependencies
bootmod.headers:	dm_generic.headers
l4util.lib:		rmgr.headers rmgr.lib
con.headers:		l4sys.headers dm_generic.headers
dm_generic.headers:	l4sys.headers l4env.headers
dm_mem.headers:		dm_generic.headers
dm_phys.headers:	dm_mem.headers
semaphore.headers:	l4sys.headers l4util.headers l4env.headers thread.headers
thread.headers:		l4sys.headers l4env.headers
dsi_example.headers:	l4sys.headers
dsi_staticfs.headers:	l4sys.headers
dsitest_receiver.headers: l4sys.headers
dsitest_sender.headers:	l4sys.headers
exec.headers:		l4sys.headers dm_generic.headers
generic_blk.headers:	l4sys.headers semaphore.headers dm_generic.headers
generic_fprov.headers:	l4sys.headers dm_generic.headers
loader.headers:		l4sys.headers dm_generic.headers
qos_mgr.headers:	l4sys.headers
qos_quota.headers:	qos_mgr.headers
qos_rmgr.headers:	qos_mgr.headers
qos_user.headers:	l4sys.headers
loader.lib:		bootmod.lib con.lib dm_generic.lib dm_mem.lib \
			generic_ts.lib l4env.lib l4rm.lib l4util.lib log.lib \
			names.lib rmgr.lib semaphore.lib slab.lib thread.lib
usb.bin:		dope.bin

# The directories/packages of l4env
L4ENV_SUBDIRS = \
	bootmod \
	buffermgr \
	con \
	crtx \
	cxx \
	dm_generic \
	dm_mem \
	dm_phys \
	exec \
	generic_blk \
	generic_fprov \
	generic_io \
	generic_ts \
	input \
	l4env \
	linux-24-headers \
	l4io \
	l4rm \
	l4sys \
	l4util \
	loader \
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
	Makeconf.stdtargets		\
	Makefile

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
# Rules begin.
# We start with cont-checking, as this requires special treatment
ifeq ($(filter cont,$(MAKECMDGOALS)),)

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

TGT_bin		:= $(addsuffix .bin,$(filter-out $(L4ENV_SUBDIRS),$(BUILD_SUBDIRS)))
TGT_lib		:= $(addsuffix .lib,$(filter-out $(L4ENV_SUBDIRS),$(BUILD_SUBDIRS)))
TGT_headers	:= $(addsuffix .headers,$(filter-out $(L4ENV_SUBDIRS),$(BUILD_SUBDIRS)))
TGT_l4env	:= $(addsuffix .bin,$(L4ENV_BUILD_SUBDIRS))
TGT_l4env-lib	:= $(addsuffix .lib,$(L4ENV_BUILD_SUBDIRS))
TGT_l4env-headers:=$(addsuffix .headers,$(L4ENV_BUILD_SUBDIRS))

bin:			$(TGT_bin)
lib $(TGT_bin):		$(TGT_lib)
headers $(TGT_lib): 	$(TGT_headers)
l4env $(TGT_headers): 	$(TGT_l4env)
l4env-lib $(TGT_l4env):	$(TGT_l4env-lib)
l4env-headers $(TGT_l4env-lib): $(TGT_l4env-headers)
$(TGT_l4env-headers):	rm-old-links

#####################
# cont handling
-include $(BID_STATE_FILE)
BID_cont_reset:
	$(VERBOSE)$(RM) $(BID_STATE_FILE)
BID_SAFE_STATE= @echo 'BID_STATE_DONE+=$@'>>$(BID_STATE_FILE)

.PHONY: all BID_cont_reset cont bin lib headers idl include doc
.PHONY: l4env l4env-lib l4env-headers l4env-include l4env-idl
.PHONY: rm-old-links

ifneq ($(filter true y,$(SHOWMESSAGES)),)
BIN_MESSAGE=@echo "=== Building servers, examples of \"$(basename $@)\" ==="
DOC_MESSAGE=echo "=== Creating documentation for package \"$(1)\" ==="
HDR_MESSAGE=@echo "=== Building IDL & includes of \"$(basename $@)\" ==="
INST_MESSAGE=echo "=== Installing Package \"$(1)\" ==="
LIB_MESSAGE=@echo "=== Building lib of \"$(basename $@)\" ==="
PURGE_INC_MESSAGE=@echo "=== Purging pending links in $(L4DIR)/include ==="
PURGE_LIB_MESSAGE=@echo "=== Purging pending links in $(L4DIR)/lib ==="
TEMPLATE_MESSAGE=echo "=== Creating symlinks to Makefile template in $(@D) ==="
UPDATE_MESSAGE=echo "=== Updating \"$(1)\" ==="
else
# bash has problems with empty '; ;' sequences
BIN_MESSAGE=true
DOC_MESSAGE=true
HDR_MESSAGE=true
INST_MESSAGE=true
LIB_MESSAGE=true
PURGE_INC_MESSAGE=true
PURGE_LIB_MESSAGE=true
UPDATE_MESSAGE=true
TEMPLATE_MESSAGE=true
endif

$(addsuffix .bin,$(BUILD_SUBDIRS)):%.bin:%/Makefile BID_cont_reset
	$(BIN_MESSAGE)	
	$(VERBOSE)$(MAKE) -C $(basename $@) all -o lib -o include -o idl
	$(VERBOSE)$(BID_SAFE_STATE)

$(addsuffix .lib,$(BUILD_SUBDIRS)):%.lib:%/Makefile BID_cont_reset
	$(LIB_MESSAGE)
	$(VERBOSE)$(MAKE) -C $(basename $@) lib -o include -o idl
	$(VERBOSE)$(BID_SAFE_STATE)

$(addsuffix .headers,$(BUILD_SUBDIRS)):%.headers:%/Makefile BID_cont_reset
	$(HDR_MESSAGE)
	$(VERBOSE)$(MAKE) -C $(basename $@) idl
	$(VERBOSE)$(BID_SAFE_STATE)

install:: $(PKG_MAKEFILE_SYMLINKS)
	$(VERBOSE)set -e; for i in $(BUILD_SUBDIRS); do \
	  $(call INST_MESSAGE,$(BID_DOLLARQUOTE)$$i); \
	  $(MAKE) -C $$i $@; \
	done

doc: $(PKG_MAKEFILE_SYMLINKS)
	$(VERBOSE)set -e; for i in $(BUILD_SUBDIRS); do \
	  $(call DOC_MESSAGE,$(BID_DOLLARQUOTE)$$i); \
	  $(MAKE) -C $$i $@; \
	done

.PHONY: $(foreach ext, .bin .lib .include .idl .headers,addsuffix $(ext), $(BUILD_SUBDIRS))

clean cleanall:: BID_cont_reset
clean cleanall::
	$(VERBOSE)$(RM) update.log
	$(VERBOSE)for i in $(BUILD_SUBDIRS) $(OBSOLETE_SUBDIRS); do \
	  echo "=== Cleaning in package  \"$$i\" ==="; \
	  if [ -r $$i/Makefile ] ; then $(MAKE) -C $$i $@; fi ; \
	done
	$(VERBOSE)$(MAKE) rm-old-links

del-backup:
	$(VERBOSE)rm -i `find . -name "*~"` `find . -name "#*#"`

pkg-links: $(PKG_MAKEFILE_SYMLINKS)

$(PKG_MAKEFILE_SYMLINKS):
	$(VERBOSE)$(TEMPLATE_MESSAGE)
	$(VERBOSE)$(LN) -s ../pkg.Makefile.tmpl $@

rm-old-links:
	$(PURGE_INC_MESSAGE)
	$(VERBOSE)test ! -d $(L4DIR)/include || find $(L4DIR)/include -type l -a ! -xtype f -a ! -xtype d -exec rm -v '{}' ';'
	$(PURGE_LIB_MESSAGE)
	$(VERBOSE)test ! -d $(L4DIR)/lib || find $(L4DIR)/lib -type l -a ! -xtype f -a ! -xtype d -exec rm -v '{}' ';'

rm-pkg-links:
	$(VERBOSE)find -mindepth 2 -maxdepth 2 -type l -name Makefile -exec $(RM) -i '{}' ';'

rm-pkg-links-force:
	$(VERBOSE)find -mindepth 2 -maxdepth 2 -type l -name Makefile -exec $(RM) '{}' ';'


# Update Makefile on l4env_update.
ifneq ($(filter l4env_update update,$(MAKECMDGOALS)),)
.PHONY: Makefile
Makefile: update.log
	cvs up $@ 2>&1 | tee -a update.log
endif

.PHONY: update.log
update.log:
	$(VERBOSE)$(RM) $@

l4env_update: l4conf_update gcc-wrap_update dice_update BID_cont_reset update.log
	$(VERBOSE)set -e; for i in $(L4ENV_SUBDIRS); do \
	  if [ "$$i" != linux-24-headers -o -d $$i -o \( "$(LINUX24_AVAIL)" = "n" -a "$(LINUX24_GLOBALINC)" = "n" \) ] ; then \
	    $(call UPDATE_MESSAGE,$(BID_DOLLARQUOTE)$$i); \
	    cvs -q update -d $$i 2>&1 | tee -a update.log; \
	  fi; \
	done
	cvs update pkg.Makefile.tmpl | tee -a update.log

update: l4env_update update.log
	$(VERBOSE)set -e ; \
	  for i in $(filter-out $(L4ENV_SUBDIRS),$(ALL_SUBDIRS)) ; do \
	    $(call UPDATE_MESSAGE,$(BID_DOLLARQUOTE)$$i); \
	    cvs -q update -d $$i 2>&1 | tee -a update.log; done

gcc-wrap:
	$(VERBOSE)$(MAKE) -C $(L4DIR)/tool/gcc-wrap

gcc-wrap_update: update.log
	$(VERBOSE)if [ -d $(L4DIR)/tool/gcc-wrap ] ; then \
		$(call UPDATE_MESSAGE,l4/tool/gcc-wrap); \
		cd $(L4DIR) && \
		cvs -q update -d tool/gcc-wrap 2>&1 | tee -a update.log; \
	fi

dice_update: update.log
	$(VERBOSE)if [ -d $(L4DIR)/tool/dice ] ; then \
		$(call UPDATE_MESSAGE,l4/tool/dice); \
		cd $(L4DIR) && \
		cvs -q update -d tool/dice 2>&1 | tee -a update.log; \
	fi

l4conf_update: update.log
	@$(call UPDATE_MESSAGE,l4/Make{confs/files})
	$(VERBOSE)cd .. && \
		cvs -q update $(L4CONF_FILES) 2>&1 | tee -a update.log
	@$(call UPDATE_MESSAGE,l4/mk/)
	$(VERBOSE)cd ../mk && \
		cvs -q update -d 2>&1 | tee -a update.log

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
	     if [ -f $$i/server/Makefile ]; then			\
	       echo "=== Re-linking \"$$i\" server ===";		\
	       $(MAKE) -C $$i/server relink;				\
	     else			 				\
	       echo "( Package \"$$i\" has no server dir )"; 	\
	     fi;							\
	     if [ -f $$i/examples/Makefile ]; then			\
	       echo "=== Re-linking \"$$i\" examples ===";		\
	       $(MAKE) -C $$i/examples relink;				\
	     else							\
	       echo "( Package \"$$i\" has no examples dir )";		\
	     fi;							\
	 done;								\

# global defines

.PHONY:		install clean cleanall kernels


print-subdirs:
	@echo $(BUILD_SUBDIRS)
	@echo obsolete: $(OBSOLETE_SUBDIRS)

help:
	@echo "Specify one of the following targets:"
	@echo "all             - build the l4env and the packages neither broken nor obsolete"
	@echo "cont            - after correcting errors, continue where \"make all\" failed"
	@echo "doc             - build documentation (not build per default)"
	@echo "install         - install the packages, use only after making all!"
	@echo
	@echo "clean           - clean the packages"
	@echo "cleanall        - clean the packages pedanticly"
	@echo "del-backup      - remove temporary file in this tree"
	@echo "headers         - short for includes + idl"
	@echo "idl             - generate IDLs in the subdirs and link them to the L4DIR"
	@echo "includes        - link the includes in the subdirs to the L4DIR"
	@echo "lib             - generate the libraries in the subdirs and link them"
	@echo "l4conf_update   - cvsupdate the diverse Makeconf.* in the L4DIR"
	@echo "l4env           - build the l4env"
	@echo "l4env-headers   - headers target for the L4ENV dirs"
	@echo "l4env-idl       - idl target for the L4ENV dirs"
	@echo "l4env-include   - include target for the L4ENV dirs"
	@echo "l4env-lib       - lib target for the L4ENV dirs"
	@echo "l4env_update    - cvsupdate of the L4ENV dirs"
	@echo "pkg-links       - create symlinks to pkg.Makefile template in all subdirs"
	@echo "print-subdirs   - print the SUBDIRS which are subject to build"
	@echo "relink          - rebuild servers and examples"
	@echo "relocate        - building the relocatable binaries in the dirs"
	@echo "reset           - reset the state used by the \"cont\" target"
	@echo 'rm-old-links    - remove invalid symlinks in $$(L4DIR)/lib, $$(L4DIR)/include'
	@echo "rm-pkg-links    - delete symlinked Makefiles in the subdirs interactively"
	@echo "rm-pkg-links-force - dito, non-interactively"
	@echo "update          - update l4env + the packages currently checked out"
	@echo "help            - this help"
	@echo -e "\nWhen building, you can omit sub-targets using the \"-o <tgt>\" switch to make."
	@echo "\"make -o l4env\" skips rebuilding the l4env, \"make -o lib\" only builds"
	@echo "binaries and examples."

#####################
# cont handling: "cont" is in the target list
else
-include $(BID_STATE_FILE)
cont:
	$(VERBOSE)$(MAKE) $(addprefix -o ,BID_cont_reset $(BID_STATE_DONE)) \
		$(filter-out cont,$(MAKECMDGOALS))

# eat up the other targets
.DEFAULT:
	@true
endif 

