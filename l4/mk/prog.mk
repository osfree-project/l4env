# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Template for binary directories
#
# $Id$
#
# $Author$
#
# Makeconf is used, see there for further documentation
# install.inc is used, see there for further documentation
# binary.inc is used, see there for further documentation

ifeq ($(origin _L4DIR_MK_PROG_MK),undefined)
_L4DIR_MK_PROG_MK=y

ROLE = prog.mk

include $(L4DIR)/mk/Makeconf

# define INSTALLDIRs prior to including install.inc, where the install-
# rules are defined.
INSTALLDIR_BIN		?= $(DROPS_STDDIR)/bin/$(subst -,/,$(SYSTEM))
INSTALLDIR_BIN_LOCAL	?= $(OBJ_BASE)/bin/$(subst -,/,$(SYSTEM))
ifeq ($(BID_STRIP_PROGS),y)
INSTALLFILE_BIN 	?= $(STRIP) --strip-unneeded $(1) -o $(2) && \
			   chmod 755 $(2)
INSTALLFILE_BIN_LOCAL 	?= $(STRIP) --strip-unneeded $(1) -o $(2) && \
			   chmod 755 $(2)
else
INSTALLFILE_BIN 	?= $(INSTALL) -m 755 $(1) $(2)
INSTALLFILE_BIN_LOCAL 	?= $(INSTALL) -m 755 $(1) $(2)
endif

INSTALLFILE		= $(INSTALLFILE_BIN)
INSTALLDIR		= $(INSTALLDIR_BIN)
INSTALLFILE_LOCAL	= $(INSTALLFILE_BIN_LOCAL)
INSTALLDIR_LOCAL	= $(INSTALLDIR_BIN_LOCAL)

# our mode
MODE 			?= l4env

# include all Makeconf.locals, define common rules/variables
include $(L4DIR)/mk/binary.inc

ifneq ($(SYSTEM),) # if we have a system, really build

TARGET_STANDARD := $(TARGET) $(TARGET_$(OSYSTEM))
TARGET_PROFILE := $(addsuffix .pr,$(filter $(BUILD_PROFILE),$(TARGET)))
TARGET	+= $(TARGET_$(OSYSTEM)) $(TARGET_PROFILE)

# define some variables different for lib.mk and prog.mk
ifeq ($(MODE),loader)
LDFLAGS += -Wl,--dynamic-linker,libld-l4.s.so
endif
ifeq ($(BID_GENERATE_MAPFILE),y)
LDFLAGS += -Wl,-Map -Wl,$(strip $@).map
endif
LDFLAGS += $(addprefix -L, $(PRIVATE_LIBDIR) $(PRIVATE_LIBDIR_$(OSYSTEM)) $(PRIVATE_LIBDIR_$@) $(PRIVATE_LIBDIR_$@_$(OSYSTEM)))
LDFLAGS += $(addprefix -L, $(L4LIBDIR)) $(LIBCLIBDIR)
LDFLAGS	+= $(addprefix -T, $(LDSCRIPT)) $(LIBS) $(L4LIBS) $(LIBCLIBS) $(LDFLAGS_$@)
# Not all host linkers understand this option
ifneq ($(MODE),host)
LDFLAGS += -Wl,--warn-common
endif

ifeq ($(notdir $(LDSCRIPT)),main_stat.ld)
# ld denies -gc-section when linking against shared libraries
ifeq ($(findstring FOO,$(patsubst -l%.s,FOO,$(LIBS) $(L4LIBS) $(LIBCLIBS))),)
LDFLAGS += -Wl,-gc-sections
endif
endif

include $(L4DIR)/mk/install.inc

#VPATHEX = $(foreach obj, $(OBJS), $(firstword $(foreach dir, \
#          . $(VPATH),$(wildcard $(dir)/$(obj)))))

# target-rule:

# Make looks for obj-files using VPATH only when looking for dependencies
# and applying implicit rules. Though make adapts its automatic variables,
# we cannot use them: The dependencies contain files which have not to be
# linked to the binary. Therefore the foreach searches the obj-files like
# make does, using the VPATH variable.
# Use a surrounding strip call to avoid ugly linebreaks in the commands
# output.

# Dependencies: When we have ld.so, we use MAKEDEP to build our
# library dependencies. If not, we fall back to LIBDEPS, an
# approximation of the correct dependencies for the binary. Note, that
# MAKEDEP will be empty if we dont have ld.so, LIBDEPS will be empty
# if we have ld.so.

ifeq ($(HAVE_LDSO),)
LIBDEPS = $(foreach file, \
                    $(patsubst -l%,lib%.a,$(filter-out -L%,$(LDFLAGS))) \
                    $(patsubst -l%,lib%.so,$(filter-out -L%,$(LDFLAGS))),\
                    $(word 1, $(foreach dir, \
                           $(patsubst -L%,%,\
                           $(filter -L%,$(LDFLAGS) $(L4ALL_LIBDIR))),\
                      $(wildcard $(dir)/$(file)))))
endif

DEPS	+= $(foreach file,$(TARGET), $(dir $(file)).$(notdir $(file)).d)

# Link C++ programs with g++ (esp. in l4linux mode)
LINK_PROGRAM := $(CC)
ifneq ($(SRC_CC),)
LINK_PROGRAM := $(CXX)
endif

$(TARGET): $(OBJS) $(LIBDEPS) $(CRT0) $(CRTN)
	@$(LINK_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_LD_NAME)) $(LINK_PROGRAM) -o $@ $(CRT0) $(OBJS) $(LDFLAGS) $(CRTN)
	@$(BUILT_MESSAGE)

endif	# architecture is defined, really build

-include $(DEPSVAR)
.PHONY: all clean cleanall config help install oldconfig txtconfig
help::
	@echo "  all            - compile and install the binaries"
ifneq ($(SYSTEM),)
	@echo "                   to $(INSTALLDIR_LOCAL)"
endif
	@echo "  install        - compile and install the binaries"
ifneq ($(SYSTEM),)
	@echo "                   to $(INSTALLDIR)"
endif
	@echo "  relink         - relink and install the binaries"
ifneq ($(SYSTEM),)
	@echo "                   to $(INSTALLDIR_LOCAL)"
endif
	@echo "  scrub          - delete backup and temporary files"
	@echo "  clean          - delete generated object files"
	@echo "  cleanall       - delete all generated, backup and temporary files"
	@echo "  help           - this help"
	@echo
ifneq ($(SYSTEM),)
	@echo "  binaries are: $(TARGET)"
else
	@echo "  build for architectures: $(TARGET_SYSTEMS)"
endif

endif	# _L4DIR_MK_PROG_MK undefined
