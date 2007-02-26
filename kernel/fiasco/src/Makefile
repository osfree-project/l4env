srcdir		?= NOT_SET
tooldir		:= $(srcdir)/../tool

CONFIG_BANNER_STRING ?= "Fiasco - prepare for world domination"

.PHONY: all do-all test-all config menuconfig xconfig oldconfig regenconfig \
	mrproper doc help update

all:

help:
	@echo "Possible targets are:"
	@echo "  menuconfig     - configure Fiasco (ncurses mode)"
	@echo "  config         - like menuconfig"
	@echo "  xconfig        - configure Fiasco (tk mode)"
	@echo "  all            - Fiasco binary"
	@echo "  clean          - clear all auto-generated files in auto"
	@echo "  cleanall       - like clean + dependencies"
	@echo "  mrproper       - like cleanall + config files"
	@echo "  update         - update Fiasco and preprocess using cvs"
	@echo "  DEPS           - dependencies between kernel object files"
	@echo "  DEPS.ps        - graphical representation of DEPS"
	@echo "  DEPS.tred.ps   - transitive reduction of DEPS.ps"
	@echo "  doc            - doxygen HTML documentation into docs/"
	@echo "  TAGS tags      - create tags files"

ifneq ($(MAKECMDGOALS),help)

ifeq ($(srcdir),NOT_SET)
all config menuconfig xconfig oldconfig:
	@echo "======================================================================"
	@echo " Building Fiasco in the src directory is not possible!"
	@echo ""
	@echo " Go to the Fiasco root directory and create your build directory with"
	@echo "    cd .. && make BUILDDIR=build-dir"
	@echo "======================================================================"
	@exit 1

else # srcdir != NOT_SET

all: globalconfig.out

ifeq ($(filter config xconfig menuconfig oldconfig,$(MAKECMDGOALS)),)

-include globalconfig.out

# use patsubst here to prevent confusion of syntax highlighting editors :-)
CONFIG_XARCH	:= $(patsubst "%",%,$(CONFIG_XARCH))
CONFIG_ABI	:= $(patsubst "%",%,$(CONFIG_ABI))

ifeq ("$(CONFIG_XARCH)","")

all: menuconfig
	@echo "========================================================="
	@echo "Now run make again to build!"
	@echo "========================================================="
	@exit 1

else # ! no XARCH
ifeq ("$(CONFIG_ABI)","")
all:
	@echo "========================================================="
	@echo "ERROR: No ABI version set (run 'make menuconfig')!"
	@echo "========================================================="
	@exit 1
else # ! no ABI

# 
# At this point, globalconfig.out is up-to-date.  Update Modules and
# .Modules.deps, then restart using Makefile.sub1, Makefile.sub2.
#

# Read Make configuration
include $(srcdir)/Makeconf

$(MODULES_FILE): $(srcdir)/$(MODULES_FILE) 
	@ updating=false; \
	  if [ -f $@ -a -f $< -a $< -nt $@ ]; \
	  then \
	    if [ -f $@.md5 ] && $(MD5SUM) $@ | cmp -s - $@.md5; \
	    then \
	      echo "\"$@\" is unchanged -- updating from \"$<\""; \
	      rm -f $@ $@.md5; \
	      updating=true; \
	    else \
	      echo "\"$<\" is newer, but \"$@\" is changed -- please update or remove \"$@\""; \
	      exit 1; \
	    fi; \
	  fi; \
	  if [ ! -f $@ ]; \
	  then \
	    $$updating || \
	      echo "No user-specific modules file found -- trying \"$<\""; \
	    cp $< $@; \
	    $(MD5SUM) $@ > $@.md5.new && mv $@.md5.new $@.md5; \
	  fi
# Modules.md5 must not be an extra make target.  We must create the
# checksum file at the instant we copy Modules from Modules.<arch>,
# and this invariant cannot be ensured with an extra make target.

$(srcdir)/$(MODULES_FILE):

-include $(MODULES_FILE)
ifdef SUBSYSTEMS
 _modules_read_ = true
endif

ifdef _modules_read_
GENERATED_MODULES = $(foreach subsys, $(SUBSYSTEMS), \
		      $(INTERFACES_$(subsys)))
ALL = $(foreach subsys, $(SUBSYSTEMS), $($(subsys)) $($(subsys)_EXTRA))

$(foreach m, $(GENERATED_MODULES), auto/stamp-$(m).ready): $(MODULES_FILE)

.PRECIOUS: .Modules.deps
.Modules.deps: $(MODULES_FILE) globalconfig.out
	@mkdir -p auto
	@echo "Creating $@"
	@($(foreach mod, $(GENERATED_MODULES),				\
	    echo 'auto/stamp-$(mod).ready:				\
		  $(addsuffix .cpp,$(call eval_impl,$(mod)))';		\
	    echo '$(patsubst %,auto/%.cc,$(call eval_impl,$(mod)))'	\
		 'auto/$(mod).h auto/$(mod)_i.h:			\
		  auto/stamp-$(mod).ready ;				\
	          @[ -e $$@ ] || { $$(RM) $$<; $$(MAKE) $$<; }';	\
	  )) > $@.new
	@($(foreach subsys, $(SUBSYSTEMS),				      \
	    echo 'IFDEPS += $(addprefix ., $(addsuffix .cc.d,		      \
			      $(foreach in,$(INTERFACES_$(subsys)),	      \
				$(call eval_impl,$(in)))))' ;		      \
	    echo 'CXXSRC_$(subsys) += $(addsuffix .cc,			      \
					$(foreach in,$(INTERFACES_$(subsys)), \
					  $(call eval_impl,$(in))))';	      \
	    echo 'OBJ_$(subsys) += $$(CXXSRC_$(subsys):.cc=.o)		      \
		                   $$(CSRC_$(subsys):.c=.o)		      \
		                   $$(ASSRC_$(subsys):.S=.o)' ; ) ) >> $@.new
	@echo "GENERATED_MODULES = $(GENERATED_MODULES)" >> $@.new
	@echo "ALL = $(ALL)" >> $@.new
	@echo "_modules_deps_read_ = true" >> $@.new
	@mv $@.new $@

endif # _modules_read_

# 
# Makefile.sub1: Create source files.
# 

.PHONY: create-sources
create-sources: $(MODULES_FILE) globalconfig.out .Modules.deps 
	$(MAKE) srcdir=$(srcdir) -f $(srcdir)/Makefile.sub1 
auto/stamp-%.ready: $(MODULES_FILE) globalconfig.out .Modules.deps 
	$(MAKE) srcdir=$(srcdir) -f $(srcdir)/Makefile.sub1 $@

#
# Makefile.sub2: Create everything else.
#
all doc DEPS DEPS.ps DEPS.a4.ps DEPS.tred.ps TAGS tags %.o %_t: \
  $(MODULES_FILE) .Modules.deps create-sources globalconfig.h
	$(MAKE) srcdir=$(srcdir) -f $(srcdir)/Makefile.sub2 $@

# Divert any target we do not explicitly mention in this Makefile to
# Makefile.sub2.  (Unfortunately 1, this does not work for file
# targets that already exist in this directory.  Unfortunately 2,
# .DEFAULT does not accept prerequisites, so we must "make
# create-sources" manually.)
.DEFAULT: 
	$(MAKE) create-sources
	$(MAKE) srcdir=$(srcdir) -f $(srcdir)/Makefile.sub2 $@

# Well, we need to provide some empty rules for some targets to
# prevent the above catch-all from running amok.
Makeconf.local Makerules.local $(srcdir)/Makeconf.local: ; 

%: %.o				# delete implicit rule

endif # ! no ABI
endif # ! no XARCH
endif # ! config xconfig menuconfig oldconfig

auto:
	test -e auto || mkdir auto

rules.out: $(srcdir)/rules.cml
	@echo =============================================
	@echo rules.out was outdated, do \'make menuconfig\'
	@echo to see if there are new config options!
	@echo =============================================
	$(tooldir)/cml2/cmlcompile.py -o $@ $<

globalconfig.out: rules.out
	$(tooldir)/cml2/cmlconfigure.py -b -B $(CONFIG_BANNER_STRING) \
		-c -i globalconfig.out -o globalconfig.out $<

globalconfig.h: globalconfig.out
	$(tooldir)/cml2/configtrans.py -h $@ $<

config menuconfig: rules.out
	$(tooldir)/cml2/cmlconfigure.py -B $(CONFIG_BANNER_STRING) \
		-c -i globalconfig.out -o globalconfig.out $<

oldconfig: rules.out
	$(tooldir)/cml2/cmlconfigure.py -B $(CONFIG_BANNER_STRING) \
		-t -I globalconfig.out -o globalconfig.out $<

regenconfig: rules.out
	$(tooldir)/cml2/cmlconfigure.py -B $(CONFIG_BANNER_STRING) \
		-b -I globalconfig.out -o globalconfig.out $<

xconfig: rules.out
	$(tooldir)/cml2/cmlconfigure.py -x -i globalconfig.out \
		-o globalconfig.out $<


ifneq ($(filter clean cleanall mrproper,$(MAKECMDGOALS)),)

# Try to suck in clean targets from subsystems' Makefile fragments
ifdef _modules_read_
-include $(foreach subsys, $(SUBSYSTEMS), $(srcdir)/Makerules.$(subsys))
endif

.DEFAULT:

.PHONY: clean cleanall mrproper \
	$(foreach subsys, $(SUBSYSTEMS), clean-$(subsys)) \
	$(foreach subsys, $(SUBSYSTEMS), cleanall-$(subsys))

clean: $(foreach subsys, $(SUBSYSTEMS), clean-$(subsys))
	$(RM) $(ALL)
	$(RM) *.o fiasco
	$(RM) auto/*.cc auto/*.h auto/*.S auto/stamp-*.ready
	$(RM) .Clean-auto .Compiler-config

cleanall: clean $(foreach subsys, $(SUBSYSTEMS), cleanall-$(subsys))
	$(foreach subdir, $(SUBDIRS), $(RM) $(subdir)/{.,}*.d)
	$(RM) rules.out
	$(RM) {.,}*.d *~ globalconfig.{h,h.old} Circular
	$(RM) .Modules.deps

mrproper: cleanall
	$(RM_R) globalconfig.out Modules.* DEPS*
	$(RM_R) auto docs

endif # clean, cleanall, mrproper

.PHONY: update
update:
	cd $(srcdir)/.. && cvs -q update
	cd $(L4DIR)/tool/preprocess && cvs -q update

endif # srcdir != NOT_SET

endif # MAKECMDGOALS != help

