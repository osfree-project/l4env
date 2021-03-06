# $Id$
# GLOBAL Makefile for all packages
#
# DO NOT EDIT -- Automatically generated by Adam's configuration system!!!
#

IGNORE_OBJDIR_TARGETS := update l4env_update

L4DIR ?= ..
include $(L4DIR)/mk/Makeconf

# other (mostly IDL-related) dependencies
l4con.headers:		dm_generic.headers
dm_generic.headers:	l4env.headers
dm_mem.headers:		dm_generic.headers
dm_phys.headers:	dm_mem.headers
semaphore.headers:	l4util.headers l4env.headers thread.headers
thread.headers:		l4env.headers
generic_blk.headers:	semaphore.headers dm_generic.headers
generic_fprov.headers:	dm_generic.headers
loader.headers:		dm_generic.headers
qos_quota.headers:	qos_mgr.headers
qos_rmgr.headers:	qos_mgr.headers
dm_qos.headers:		qos_rmgr.headers
local_socks.headers:	l4vfs.headers
rt_mon.headers:		l4vfs.headers
l4vfs_proxyfs.headers:	l4vfs.headers
lock.headers:		semaphore.headers
loader.lib:		semaphore.lib thread.lib dm_generic.lib dm_mem.lib \
			slab.lib l4env.lib l4rm.lib log.lib names.lib \
			roottask.lib generic_ts.lib l4util.lib libsigma0.lib \
			l4sys.lib libc_backends_l4env.lib crtx.lib \
			generic_fprov.lib uclibc.lib libc_support.lib
l4util.lib:		roottask.lib
l4io.lib:		omega0.lib
cebit04.bin:		nethub.bin
flips.headers:		l4vfs.headers
ferret.headers:		l4vfs.lib
uclibc++.headers:	uclibc.headers
cunit.lib:		dde.bin
nitpicker.bin:		dope.bin
examples.bin:		dope.bin l4con.bin
lyon.headers:		stpm.headers
stpm.headers:		libcrypto.headers
stpm.lib:		dde.lib
tftp.bin:		ore.lib
log.headers:		uclibc.headers
bootstrap.bin:		sigma0.bin
$(addsuffix .lib,$(filter-out uclibc,$(BUILD_SUBDIRS))): uclibc.lib

# The directories/packages of l4env
L4ENV_SUBDIRS = \
	bootstrap \
	crtx \
	cxx \
	demangle \
	dm_generic \
	dm_mem \
	dm_phys \
	drivers \
	events \
	generic_fprov \
	generic_io \
	generic_ts \
	input \
	ipcmon \
	l4env \
	libsigma0 \
	linux-24-headers \
	l4con \
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
	semaphore \
	serial \
	sigma0 \
	simple_ts \
	slab \
	task \
	thread \
	x86emu \
	libc_support \
	roottask \
	libc_backends \
	libc_backends_l4env \
	lxfuxlibc \
	zlib \
	uclibc uclibc_r uclibc++


ifeq ($(USE_TASKLIB),y)
roottask.headers:	task.headers
roottask.lib:		task.lib
loader.lib:		task.lib
L4ENV_SUBDIRS += task
endif

ifeq ($(BUILD_ARCH_arm),y)
L4ENV_SUBDIRS += arm_drivers
endif

# -----------------------------

BID_STATE_FILE = $(OBJ_BASE)/pkg/BUILD.state

# all our packages
ALL_SUBDIRS	= $(shell find -L . -maxdepth 1 -type d ! -name .svn -printf %P' ')

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
$(filter-out l4sys.headers,$(TGT_l4env-headers)): l4sys.headers
l4sys.headers:		rm-old-links

#####################
# cont handling
-include $(BID_STATE_FILE)
BID_cont_reset:
	$(VERBOSE)$(RM) $(BID_STATE_FILE)
BID_SAFE_STATE= @echo 'BID_STATE_DONE+=$@'>>$(BID_STATE_FILE)

.PHONY: all BID_cont_reset cont bin lib headers idl include doc ptest
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
UPDATE_MESSAGE=echo -e $(EMPHSTART)"=== Updating \"$(1)\" ==="$(EMPHSTOP)
PTEST_MESSAGE=echo "=== Testing Package \"$(1)\" ==="
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
PTEST_MESSAGE=true
endif

$(addsuffix .bin,$(BUILD_SUBDIRS)):%.bin:%/Makefile BID_cont_reset
	$(BIN_MESSAGE)	
	$(VERBOSE)PWD=$(PWD)/$(basename $@) $(MAKE) -C $(basename $@) all -o lib -o include -o idl
	$(VERBOSE)$(BID_SAFE_STATE)

$(addsuffix .lib,$(BUILD_SUBDIRS)):%.lib:%/Makefile BID_cont_reset
	$(LIB_MESSAGE)
	$(VERBOSE)PWD=$(PWD)/$(basename $@) $(MAKE) -C $(basename $@) lib -o include -o idl
	$(VERBOSE)$(BID_SAFE_STATE)

$(addsuffix .headers,$(BUILD_SUBDIRS)):%.headers:%/Makefile BID_cont_reset
	$(HDR_MESSAGE)
	$(VERBOSE)PWD=$(PWD)/$(basename $@) $(MAKE) -C $(basename $@) include
	$(VERBOSE)PWD=$(PWD)/$(basename $@) $(MAKE) -C $(basename $@) idl
	$(VERBOSE)$(BID_SAFE_STATE)

install::
	$(VERBOSE)set -e; for i in $(BUILD_SUBDIRS); do \
	  $(call INST_MESSAGE,$(BID_DOLLARQUOTE)$$i); \
	  PWD=$(PWD)/$$i $(MAKE) -C $$i $@; \
	done

doc:
	$(VERBOSE)set -e; for i in $(BUILD_SUBDIRS); do \
	  $(call DOC_MESSAGE,$(BID_DOLLARQUOTE)$$i); \
	  PWD=$(PWD)/$$i $(MAKE) -C $$i $@; \
	done

ptest:
	$(VERBOSE)set -e; \
	for d in $(BUILD_SUBDIRS); do \
	  if [ -n "$$(find $$d -type d -name $@)" ]; then \
	    $(call PTEST_MESSAGE,$(BID_DOLLARQUOTE)$$d); \
	    PWD=$(PWD)/$$d $(MAKE) -C $$d $@; \
	  fi; \
	done

TAGS:
	$(ETAGS)

tags:
	$(CTAGS)

.PHONY: TAGS tags
.PHONY: $(foreach ext, .bin .lib .include .idl .headers,addsuffix $(ext), $(BUILD_SUBDIRS))

clean cleanall:: BID_cont_reset
clean cleanall::
	$(VERBOSE)$(RM) $(UPDATE_LOG)
	$(VERBOSE)for i in $(BUILD_SUBDIRS) $(OBSOLETE_SUBDIRS); do \
	  echo "=== Cleaning in package  \"$$i\" ==="; \
	  if [ -r $$i/Makefile ] ; then PWD=$(PWD)/$$i $(MAKE) -C $$i $@; fi ; \
	done
	$(VERBOSE)$(MAKE) rm-old-links

del-backup:
	$(VERBOSE)rm -i `find . -name "*~"` `find . -name "#*#"`

rm-old-links:
	$(PURGE_INC_MESSAGE)
	$(VERBOSE)test ! -d $(L4DIR)/include || \
	  find $(L4DIR)/include -type l -a ! -xtype f -a ! -xtype d \
	    -exec rm $(if $(VERBOSE),,-v) '{}' ';'
	$(PURGE_LIB_MESSAGE)
	$(VERBOSE)test ! -d $(L4DIR)/lib || \
	  find $(L4DIR)/lib -type l -a ! -xtype f -a ! -xtype d \
	    -exec rm $(if $(VERBOSE),,-v) '{}' ';'


# Update Makefile on l4env_update.
ifneq ($(filter l4env_update update,$(MAKECMDGOALS)),)
.PHONY: Makefile
Makefile Makefile.transdeps: update.log
	$(SVN) up $@ 2>&1 | tee -a $(UPDATE_LOG)
endif

UPDATE_LOG = $(call absfilename, $(L4DIR)/pkg/update.log)
.PHONY: update.log
update.log:
	$(VERBOSE)$(RM) $(UPDATE_LOG)

l4env_update: l4conf_update dice_update l4env_tools_update BID_cont_reset update.log
	$(VERBOSE)$(call UPDATE_MESSAGE,L4Env packages)
	$(VERBOSE)set -e; \
	  $(SVN) update $(sort $(L4ENV_SUBDIRS)) 2>&1 | tee -a $(UPDATE_LOG);

update: l4env_update update.log
	$(VERBOSE)$(call UPDATE_MESSAGE,Auxiliary packages)
	$(VERBOSE)set -e ; \
	    $(SVN) update $(sort $(patsubst %/.svn,%,$(wildcard $(addsuffix /.svn,$(filter-out $(L4ENV_SUBDIRS),$(ALL_SUBDIRS)))))) 2>&1 | tee -a $(UPDATE_LOG)

dice_update: update.log
	$(VERBOSE)if [ -d $(DICE_SRCDIR) ] ; then \
		$(call UPDATE_MESSAGE,$(DICE_SRCDIR)); \
		cd $(DICE_SRCDIR) && $(SVN) update 2>&1 | tee -a $(UPDATE_LOG); \
	fi

l4env_tools_update: update.log
	$(VERBOSE)if [ -d $(L4DIR)/tool/gen_dopecode ]; then \
		$(call UPDATE_MESSAGE,l4/tool/gen_dopecode); \
		cd $(L4DIR) && \
		$(SVN) update tool/gen_dopecode 2>&1 | tee -a $(UPDATE_LOG); \
	fi
	@$(call UPDATE_MESSAGE,l4/tool/gendep)
	$(VERBOSE)cd $(L4DIR) && \
		$(SVN) update tool/gendep 2>&1 | tee -a $(UPDATE_LOG)
	@$(call UPDATE_MESSAGE,l4/tool/bin)
	$(VERBOSE)cd $(L4DIR) && \
		$(SVN) update tool/bin 2>&1 | tee -a $(UPDATE_LOG)

l4conf_update: update.log
	@$(call UPDATE_MESSAGE,l4/Make{confs/files})
	$(VERBOSE)cd .. && \
		$(SVN) update Makefile 2>&1 | tee -a $(UPDATE_LOG)
	@$(call UPDATE_MESSAGE,l4/mk/)
	$(VERBOSE)cd ../mk && \
		$(SVN) update 2>&1 | tee -a $(UPDATE_LOG)

relocate:
	@for i in $(BUILD_SUBDIRS); do 					\
	     if [ -f $$i/server/src/Makefile ]; then			\
	       echo "=== Building \"$$i\" relocatable ===";		\
	       PWD=$(PWD)/$$i/server/src $(MAKE) -C $$i/server/src reloc;\
	     else			 				\
	       echo "( Package \"$$i\" has no server/src dir )"; 	\
	     fi								\
	 done;								\

relink:
	@for i in $(BUILD_SUBDIRS); do 					\
	     if [ -f $$i/server/Makefile ]; then			\
	       echo "=== Re-linking \"$$i\" server ===";		\
	       PWD=$(PWD)/$$i/server $(MAKE) -C $$i/server relink;	\
	     else			 				\
	       echo "( Package \"$$i\" has no server dir )"; 	\
	     fi;							\
	     if [ -f $$i/examples/Makefile ]; then			\
	       echo "=== Re-linking \"$$i\" examples ===";		\
	       PWD=$(PWD)/$$i/examples $(MAKE) -C $$i/examples relink;	\
	     else							\
	       echo "( Package \"$$i\" has no examples dir )";		\
	     fi;							\
	 done;								\

# global defines

.PHONY:		install clean cleanall kernels


print-subdirs:
	@echo $(BUILD_SUBDIRS)
	@echo obsolete: $(OBSOLETE_SUBDIRS)
	@echo broken: $(BROKEN_SUBDIRS)

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
	@echo "print-subdirs   - print the SUBDIRS which are subject to build"
	@echo "relink          - rebuild servers and examples"
	@echo "relocate        - building the relocatable binaries in the dirs"
	@echo "BID_cont_reset  - reset the state used by the \"cont\" target"
	@echo 'rm-old-links    - remove invalid symlinks in $$(L4DIR)/lib, $$(L4DIR)/include'
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

