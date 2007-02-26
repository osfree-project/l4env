# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Template for idl directories
#
# $Id$
#
# $Author$
#
# install.inc is used, see there for further documentation
#		- from install.inc, we use the feature to install files
#		  to the local install directory on 'all' target.
#		  Therefore, we set INSTALLDIR_LOCAL and INSTALLFILE_LOCAL.
#		  Note that we changed INSTALLDIR_IDL_LOCAL AND INSTALLDIR_IDL
# binary.inc is used, see there for further documentation

ifeq ($(origin _L4DIR_MK_IDL_MK),undefined)
_L4DIR_MK_IDL_MK=y

ROLE = idl.mk

# define LOCAL_INSTALLDIR prior to including install.inc, where the install-
# rules are defined. Same for INSTALLDIR.
INSTALLDIR_IDL		?= $(DROPS_STDDIR)/include/$(ARCH)/$(L4API)/$(INSTALL_INC_PREFIX)
INSTALLDIR_IDL_LOCAL	?= $(L4DIR)/include/$(ARCH)/$(L4API)/$(INSTALL_INC_PREFIX)
INSTALLFILE_IDL		?= $(INSTALL) -m 644 $(1) $(2)
INSTALLFILE_IDL_LOCAL	?= $(LN) -sf $(call absfilename,$(1)) $(2)

INSTALLFILE		= $(INSTALLFILE_IDL)
INSTALLDIR		= $(INSTALLDIR_IDL)
INSTALLFILE_LOCAL	= $(INSTALLFILE_IDL_LOCAL)
INSTALLDIR_LOCAL	= $(INSTALLDIR_IDL_LOCAL)

ifneq ($(origin INSTALL_INC_PREFIX), undefined)
$(warning You have overwritten INSTALL_INC_PREFIX. I hope you know what you are doing.)
endif
INSTALL_INC_PREFIX     ?= l4/$(PKGNAME)

# our default MODE is 'l4env'
MODE			?= l4env

IDL_EXPORT_STUB ?= %
IDL_EXPORT_IDL	?= %

.general.d: $(L4DIR)/mk/idl.mk
include $(L4DIR)/mk/Makeconf
include $(L4DIR)/mk/binary.inc

ifneq ($(SYSTEM),) # if we have a system, really build
#######################################################
#
# SYSTEM valid, we are in an OBJ-<xxx> system subdir
#
#######################################################

IDL_EXPORT_STUB ?= %
IDL_TYPE	?= dice

IDL_DEP =		$(addprefix .,$(addsuffix .d,$(notdir $(IDL))))
IDL_SKELETON_C =	$(IDL:.idl=-server.c)
IDL_SKELETON_H =	$(IDL_SKELETON_C:.c=.h)
IDL_STUB_C =		$(IDL:.idl=-client.c)
IDL_STUB_H = 		$(IDL_STUB_C:.c=.h)
IDL_OPCODE_H =		$(IDL:.idl=-sys.h)

IDL_FILES = $(IDL_SKELETON_C) $(IDL_SKELETON_H) $(IDL_STUB_C) $(IDL_STUB_H) \
	    $(IDL_OPCODE_H)

# Makro that expands to the list of generated files
# arg1 - name of the idl file. Path and extension will be stripped
IDL_FILES_EXPAND =	$(addprefix $(notdir $(basename $(1))),-server.c -server.h -client.c -client.h -sys.h)

INSTALL_TARGET = $(patsubst %.idl,%-sys.h,				\
		   $(filter $(IDL_EXPORT_SKELETON) $(IDL_EXPORT_STUB),$(IDL)))\
		 $(patsubst %.idl,%-server.h,				\
		   $(filter $(IDL_EXPORT_SKELETON),$(IDL)))		\
		 $(patsubst %.idl,%-client.h,				\
		   $(filter $(IDL_EXPORT_STUB), $(IDL)))

all:: $(IDL_FILES)
.DELETE_ON_ERROR:

# the dependencies for the generated files
DEPS		+= $(IDL_DEP)

ifneq (,$(filter-out corba dice, $(IDL_TYPE)))
$(error IDL_TYPE is neither <dice> nor <corba>)
endif

# the IDL file is found one directory up
vpath %.idl ..

# DICE mode
IDL_FLAGS	+= $(addprefix -P,$(CPPFLAGS))

# XXX just commented this out as long as dice does not support native x0
# say dice if we want to build sources for L4X0 API.
ifeq ($(L4API),l4x0)
ifeq ($(ARCH),arm)
IDL_FLAGS	+= -Bix0
else
IDL_FLAGS	+= -Bix0adapt
endif
endif

ifeq ($(L4API),l4v4)
IDL_FLAGS	+= -Biv4
endif

ifeq ($(L4API),l4x2)
IDL_FLAGS	+= -Bix2
endif

ifeq ($(L4API),linux)
IDL_FLAGS	+= -Bisock
endif

ifeq ($(ARCH),arm)
IDL_FLAGS	+= -Bparm
endif

ifeq ($(IDL_TYPE),corba)
IDL_FLAGS	+= -C
endif

%-server.c %-server.h %-client.c %-client.h %-sys.h: %.idl .general.d
	@$(GEN_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(DICE_CPP_NAME),"$(call IDL_FILES_EXPAND,$<)",.$(<F).d) CC=$(CC_$(ARCH)) $(DICE) $(IDL_FLAGS) $<
	$(DEPEND_VERBOSE)$(ECHO) "$(call IDL_FILES_EXPAND,$<): $(DICE) $<" >>.$(<F).d
	$(DEPEND_VERBOSE)$(ECHO) "$(DICE) $<:" >>.$(<F).d


clean cleanall::
	$(VERBOSE)$(RM) $(wildcard $(addprefix $(INSTALLDIR_LOCAL)/, $(IDL_FILES)))
	$(VERBOSE)$(RM) $(wildcard $(IDL_FILES) *.aoi *.prc)

# include install.inc to define install rules
include $(L4DIR)/mk/install.inc

else
#####################################################
#
# No SYSTEM defined, we are in the idl directory
#
#####################################################

# we install the IDL-files specified in IDL_EXPORT_IDL
INSTALL_TARGET = $(filter $(IDL_EXPORT_IDL), $(IDL))

# include install.inc to define install rules
include $(L4DIR)/mk/install.inc

# install idl-files before going down to subdirs
$(foreach arch,$(TARGET_SYSTEMS), OBJ-$(arch)): $(addprefix $(INSTALLDIR_LOCAL)/,$(INSTALL_TARGET))

endif	# architecture is defined, really build
#####################################################
#
# Common part
#
#####################################################

-include $(DEPSVAR)
.PHONY: all clean cleanall config help install oldconfig reloc txtconfig

help::
	@echo "  all            - generate .c and .h from idl files and install locally"
ifneq ($(SYSTEM),)
	@echo "                   to $(INSTALLDIR_LOCAL)"
endif
	@echo "  scrub          - delete backup and temporary files"
	@echo "  clean          - delete generated source files"
	@echo "  cleanall       - delete all generated, backup and temporary files"
	@echo "  help           - this help"
	@echo
	@echo "  idls are: $(IDL)"


endif	# _L4DIR_MK_IDL_MK undefined
