# -*- Makefile -*-
# vim:set ft=make:
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Include for compiling templates (prog.mk, lib.mk)
#
# $Id$
#
# $Author$

# Makefile-Include for binary and lib directories
# Definitions for building the Makefile.inc, building dependencies,
# compiler configuration.
#
# If SYSTEM is defined and we do not clean, we generate Makefile.inc. This file
# holds the dependencies of the targets. Is also contains the definitions of
# variables holding the objects of the related targets. In a multiple
# architecture makefile, Makefile.inc will be built in arch-specific subdirs, 
# like the other components built.
#
# Most compiler- and linker-flag variables are defined in such a way that
# using them in the rule will expand all the target- and
# architecture-specific definitions.
#
# The relink-rule is defined in this file.
#
# The dependencies for compiling files are defined here, the dep-files are
# generated with names .(source-file).d for .c, .cc and .S-files.
#
# Clean-rules are defined here.
#

$(OBJ_DIR)/.general.d:	$(L4DIR)/mk/binary.inc
# our default Makefile-name in the OBJ-Subdirs
BID_OBJ_Makefile ?= Makefile

ifeq ($(SYSTEM),) # if we have no system yet, build the subdirs
#################################################################
#
# we have NO system defined in $(SYSTEM), we are in the src/ dir
#
#################################################################

# our default system
SYSTEMS	?= x86-l4v2

# intersection with BUILD_SYSTEMS

# filter the systems from the local SYSTEMS variable ($2), that match
# the build-architecture.
# args: $(1) - build architecture (one from BUILD_SYSTEMS)
#       $(2) - SYSTEMS
# 1. check, if both systems are the same (optionally reduced by the CPU)
# 2. check, if at least the arch matches (and optionally the cpu)
FILTER_SYSTEM = $(shell echo $(2)|$(AWKP) '\
			BEGIN{m=s="$(1)";sub("_[^-]*","",m)}\
			{for(i=1;i<=NF;i++){\
			 	if(m==$$i||s==$$i){print s}else\
			 	if(index(m,$$i)==1||index(s,$$i)==1)\
			 		{t=s;sub("-.*","",t);print t}}}')

# print that system of the SYSTEMS variable that actually matched with
# $(BUILD_SYSTEMS) to the given system
# args: $(1) - build architecture (SYSTEM)
#       $(2) - SYSTEMS
# + do nearly the same as in FILTER_SYSTEM, but additionally check if
#   the resulting system matches $(1). If so, print the according pattern from
#   SYSTEMS and exit
BID_ORIG_SYSTEM = $(shell echo $(2)|$(AWKP) '\
		    BEGIN{m=s="$(1)";sub("_[^-]*","",m)}\
		    {for(i=1;i<=NF;i++){\
		 	if(m==$$i||s==$$i){if(s=="$(1)"){print $$i;exit}};\
		 	if(index(m,$$i)==1||index(s,$$i)==1)\
			    {t=s;sub("-.*","",t);\
                             if(t=="$(1)"){print $$i;exit}}}}')

ifeq ($(ROLE),idl.mk)
TARGET_SYSTEMS := $(sort $(foreach sys,$(sort \
	$(shell echo $(BUILD_SYSTEMS)|$(SED) -e 's/_[^- ]*\(-\{0,1\}\)/\1/g')), \
	$(call FILTER_SYSTEM,$(sys),$(SYSTEMS))))
else
TARGET_SYSTEMS := $(sort $(foreach sys,$(BUILD_SYSTEMS),\
			$(call FILTER_SYSTEM,$(sys),$(SYSTEMS))))
endif

DIR_FROM_SUB = $(firstword $(addprefix ../,$(patsubst /%,,$(1))) $(1))
SYSTEM_TO_ARCH = $(shell echo $(1)|$(SED) -e 's/[_-].*//')
SYSTEM_TO_CPU = $(shell echo $(1)|$(SED) -ne 's/[^-_]*_\([^-]*\).*/\1/p')
SYSTEM_TO_L4API = $(shell echo $(1)|$(SED) -ne 's/.*-\(.*\)/\1/p')

$(foreach sys,$(TARGET_SYSTEMS),$(OBJ_DIR)/OBJ-$(sys)/$(BID_OBJ_Makefile)):$(OBJ_DIR)/OBJ-%/$(BID_OBJ_Makefile):$(OBJ_DIR)/.general.d
	@install -d $(dir $@)
	@echo 'L4DIR=$(L4DIR_ABS)'>$@
	@echo 'OBJ_BASE=$(OBJ_BASE)'>>$@
	@echo 'OBJ_DIR=$(OBJ_DIR)'>>$@
	@echo 'SRC_DIR=$(SRC_DIR)'>>$@
	@echo 'PKGDIR=$(PKGDIR_ABS)'>>$@
	@echo 'PKGDIR_ABS=$(PKGDIR_ABS)'>>$@
	@echo 'PKGDIR_OBJ=$(PKGDIR_OBJ)'>>$@
	@echo 'MAKECONFLOCAL=$(SRC_DIR)/Makeconf.local'>>$@
	@echo 'OSYSTEM=$(call BID_ORIG_SYSTEM,$*,$(SYSTEMS))'>>$@
	@echo 'SYSTEM=$*'>>$@
	@echo 'ARCH=$(call SYSTEM_TO_ARCH,$*)'>>$@
	@echo 'CPU=$(call SYSTEM_TO_CPU,$*)'>>$@
	@echo 'L4API=$(call SYSTEM_TO_L4API,$*)'>>$@
	@echo 'vpath %.c $(VPATH_SRC_BASE)'>>$@
	@echo 'vpath %.cc $(VPATH_SRC_BASE)'>>$@
	@echo 'vpath %.cpp $(VPATH_SRC_BASE)'>>$@
	@echo 'vpath %.S $(VPATH_SRC_BASE)'>>$@
	@echo 'vpath %.y $(VPATH_SRC_BASE)'>>$@
	@echo 'vpath %.l $(VPATH_SRC_BASE)'>>$@
	@echo 'vpath %.ld $(VPATH_SRC_BASE)'>>$@
	@echo 'vpath %.dpe $(VPATH_SRC_BASE)'>>$@
	@echo '.general.d: $(SRC_DIR)/$(if $(wildcard Make.rules),Make.rules,Makefile)'>>$@
	@echo 'include $(SRC_DIR)/$(if $(wildcard Make.rules),Make.rules,Makefile)'>>$@
	@echo 'include $$(L4DIR)/mk/$(ROLE)'>>$@

install relink scrub:: $(foreach arch,$(TARGET_SYSTEMS),\
                                          $(OBJ_DIR)/OBJ-$(arch)/$(BID_OBJ_Makefile))
	$(if $(TARGET_SYSTEMS),  $(VERBOSE)set -e ; \
	    for d in $(TARGET_SYSTEMS) ; do \
	    	PWD=$OBJ-$$d $(MAKE) -C $(OBJ_DIR)/OBJ-$$d -f $(BID_OBJ_Makefile) \
			$@ ; done )

all:: $(foreach arch,$(TARGET_SYSTEMS), $(OBJ_DIR)/OBJ-$(arch))
.PHONY: $(foreach arch,$(TARGET_SYSTEMS), $(OBJ_DIR)/OBJ-$(arch))
$(foreach arch,$(TARGET_SYSTEMS), $(OBJ_DIR)/OBJ-$(arch)):%:%/$(BID_OBJ_Makefile)
	$(VERBOSE)PWD=$@ $(MAKE) $(PL_j) -C $@ -f $(BID_OBJ_Makefile)

clean::
	$(if $(wildcard $(OBJ_DIR)/OBJ-*), $(VERBOSE)set -e ; \
	    for d in $(wildcard $(OBJ_DIR)/OBJ-*) ; do \
	        PWD=OBJ-$$d $(MAKE) -C $$d -f $(BID_OBJ_Makefile) $@; \
	    done, @true)

cleanall::
	$(VERBOSE)$(RM) -r $(wildcard $(OBJ_DIR))

.PHONY: $(TARGET_SYSTEMS)

else
###############################################################
#
# we have a system defined in $(SYSTEM), we are in an OBJ- dir
#
###############################################################

all::		$(TARGET)

L4INCDIR ?=	$(addprefix $(OBJ_BASE)/include/$(ARCH)/,$(L4API)) \
                $(addprefix $(OBJ_BASE)/include/,$(L4API)) \
                $(OBJ_BASE)/include/$(ARCH) \
                $(OBJ_BASE)/include \
		$(DICE_INCDIR) \
                $(addprefix $(DROPS_STDDIR)/include/$(ARCH)/,$(L4API)) \
                $(addprefix $(DROPS_STDDIR)/include/,$(L4API)) \
                $(DROPS_STDDIR)/include/$(ARCH) \
                $(DROPS_STDDIR)/include
L4LIBDIR ?=	$(addprefix $(OBJ_BASE)/lib/$(ARCH)_$(CPU)/,$(L4API)) \
                $(OBJ_BASE)/lib/$(ARCH)_$(CPU) \
                $(OBJ_BASE)/lib \
                $(addprefix $(DROPS_STDDIR)/lib/$(ARCH)_$(CPU)/,$(L4API)) \
                $(DROPS_STDDIR)/lib/$(ARCH)_$(CPU) \
                $(DROPS_STDDIR)/lib

L4LINUX22_DIR ?= $(L4DIR)/../linux22

#
# Variables Section
#
# There is a hierarchy on defining variables depending on the targets they
# refer to: Most standard-Make Variables are supported. This includes
# LDFLAGS	- options for ld, defined in prog.mk and lib.mk
# CPPFLAGS	- options for the c preprocessor, included in CFLAGS
# CFLAGS	- options for the c compiler
# CXXFLAGS	- options for the c++ compiler
# ASFLAGS	- options for the assembler
#
# Addtitionally, the following varibales are supported:
# SRC_C, SRC_CC, SRC_S - .c, .cc, .S source files
# CRT0, CRTN	- startup and finish code
# LIBS		- additional libs to link (with -l), including paths (-L)
# IDL		- interface definition files
# TARGET	- targets to ge built
#
# These variables will be used for all operations with the corresponding
# file types. More specific description is possible by using variables with
# added specifications. These specifications include a referred element and
# the architecture, both optional but in this order, separated by
# underscores. The referred element for CPPFLAGS, CFLAGS, CXXFLAGS and
# ASFLAGS is the source file. For the other variables, it is one of the
# target files. The TARGET variable can only be postfixed by an
# architecture.
# The specific variables will be used for the target and the referred element
# given in the name, additionally to the more general ones.
#
# Example for a valid specifications:
# SRC_C_libxverbose.a = verbose.c   - ar's verbose.o into libxverbose.a, but
#                                     not in other libs in the TARGET var.

include $(L4DIR)/mk/modes.inc

# select the variable specified in $(1) from the current architecture and
# mode. Fall back to "all" architecture if no specific version exists.
BID_mode_var= $(if $($(1)_$(ARCH)_$(MODE)),$($(1)_$(ARCH)_$(MODE)),$($(1)_all_$(MODE)))

BID_SUPPORTED ?= $(call BID_mode_var,BID_SUPPORTED)
ifneq ($(BID_SUPPORTED),y)
$(error Mode "$(MODE)" is not supported for CPU architecture "$(ARCH)")
endif

LIBCINCDIR ?= $(call BID_mode_var,LIBCINCDIR)
LIBCLIBDIR ?= $(call BID_mode_var,LIBCLIBDIR)
LIBCLIBS   ?= $(call BID_mode_var,LIBCLIBS)
L4LIBS     ?= $(call BID_mode_var,L4LIBS)
CRT0	   ?= $(call BID_mode_var,CRT0)
CRTP       ?= $(if $(filter $(CRT0_DEFAULT),$(CRT0)),$(CRTP_DEFAULT),$(CRT0))
CRTN	   ?= $(call BID_mode_var,CRTN)
LDSCRIPT   ?= $(call BID_mode_var,LDSCRIPT)

# we do not need to put our FLAG-definitions into a special file if we can
# use the target to parameterize it and if it needs not to appear in the deps 
#CRT0	=  $(word 1, $(CRT0_$@_$(OSYSTEM)) $(CRT0_$@) $(CRT0_$(OSYSTEM)))
#CRTN	=  $(word 1, $(CRTN_$@_$(OSYSTEM)) $(CRTN_$@) $(CRTN_$(OSYSTEM)))
LIBS	+= $(strip $(LIBS_$(OSYSTEM)) $(LIBS_$@) $(LIBS_$@_$(OSYSTEM)))


LDFLAGS	+= $(strip $(LDFLAGS_$(OSYSTEM)) $(LDFLAGS_$(ARCH)) $(LDFLAGS_$@) $(LDFLAGS_$@_$(OSYSTEM)))

OPTS	?= -g -O2 -fno-strict-aliasing
OPTS	+= $(CARCHFLAGS_$(ARCH)) $(CARCHFLAGS_$(ARCH)_$(CPU))
WARNINGS?= -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations $(WARNINGS_$@)

ifeq ($(MODE),host)
# never cross compile in host mode
SYSTEM_TARGET = 
CC            = $(HOST_CC)
CXX           = $(HOST_CXX)
endif


#
# These are some special defines for the X2 sources from Pistachio.
# 
DEFINES_x86-l4x2 += -DL4_ARCH_IA32 -D__L4_ARCH__=ia32
DEFINES_arm-l4x2 += -DL4_ARCH_ARM -D__L4_ARCH__=arm
DEFINES_amd64-l4x2 += -DL4_ARCH_AMD64 -D__L4_ARCH__=amd64
DEFINES_ppc32-l4x2 += -DL4_ARCH_POWERPC -D__L4_ARCH__=powerpc
DEFINES_ppc64-l4x2 += -DL4_ARCH_POWERPC64 -D__L4_ARCH__=powerpc64
DEFINES_ia64-l4x2 += -DL4_ARCH_IA64 -D__L4_ARCH__=ia64
DEFINES_alpha-l4x2 += -DL4_ARCH_ALPHA -D__L4_ARCH__=alpha

$(CLIENTIDL:.idl=-client.o): CFLAGS += -ffunction-sections
CFLAGS	+= $(BID_CFLAGS_GSTAB) $(BID_CFLAGS_OMIT_FP)
CFLAGS	+= $(CFLAGS_$(OSYSTEM)) $(CFLAGS_$(<F)) $(CFLAGS_$(<F)_$(OSYSTEM))
CFLAGS  += $(CFLAGS_$(L4API))
CFLAGS	+= $(OPTS) $(WARNINGS)
CFLAGS  += -fno-common
DEFINES += $(DEFINES_$(OSYSTEM)) $(DEFINES_$(<F)) $(DEFINES_$(<F)_$(OSYSTEM))
DEFINES += -DSYSTEM_$(subst -,_,$(SYSTEM)) -DARCH_$(ARCH) -DCPUTYPE_$(CPU) -DL4API_$(L4API)
ifdef DEBUG
ifeq (,$(filter 0 n N, $(DEBUG)))
DEFINES	+= -DDEBUG
endif
endif

CPPFLAGS+= $(CPPFLAGS_$(OSYSTEM)) $(CPPFLAGS_$(<F)) $(CPPFLAGS_$(<F)_$(OSYSTEM))
CPPFLAGS+= $(DEFINES)
CPPFLAGS+= $(BID_CPPFLAGS_SYSCALLS)
CPPFLAGS+= $(addprefix -I, $(PRIVATE_INCDIR) $(PRIVATE_INCDIR_$(OSYSTEM)) $(PRIVATE_INCDIR_$@) $(PRIVATE_INCDIR_$@_$(OSYSTEM)))
CPPFLAGS+= $(if $(CLIENTIDL)$(SERVERIDL)$(addprefix CLIENTIDL_,$(TARGET))$(addprefix SERVERDIL_,$(TARGET)),$(addprefix -I, $(IDL_PATH)))
ifneq ($(MODE),host)
CPPFLAGS+= $(addprefix -I, $(L4INCDIR))
endif
CPPFLAGS+= $(LIBCINCDIR)

CXXFLAGS+= $(BID_CFLAGS_GSTAB) $(BID_CFLAGS_OMIT_FP)
CXXFLAGS+= $(CXXFLAGS_$(OSYSTEM)) $(CXXFLAGS_$(@:.o=.cc)) $(CXXFLAGS_$(@:.o=.cc)_$(OSYSTEM))
CXXFLAGS+= $(CXXFLAGS_$(@:.o=.cpp)) $(CXXFLAGS_$(@:.o=.cpp)_$(OSYSTEM))
CXXFLAGS+= $(OPTS) $(filter-out -Wmissing-declarations -Wstrict-prototypes -Wmissing-prototypes,$(WARNINGS))
CXXFLAGS+= -fno-common

PICFLAGS = -D__PIC__ -fPIC

ASFLAGS+= $(ASFLAGS_$(ARCH)) $(ASFLAGS_$(@:.o=.S)) $(ASFLAGS_$(@:.o=.S)_$(OSYSTEM))
OBJS	+= $(strip $(OBJS_$(OSYSTEM)) $(OBJS_$@) $(OBJS_$@_$(OSYSTEM)))
ALLOBJS	= $(OBJS) $(foreach target,$(TARGET) $(TARGET_$(OSYSTEM)),\
		$(OBJS_$(target)) $(OBJS_$(target)_$(OSYSTEM)))
ALLDPI = $(sort $(foreach obj,$(ALLOBJS),$(patsubst %.dpe,%.dpi,\
		$(DPE_$(obj:.o=.c)))))

# implementation of $(CLIENTIDL) and $(SERVERIDL)
IDL_TYPE   ?= dice
IDL_PKGDIR ?= $(PKGDIR_OBJ)
IDL_PATH   ?= $(addsuffix /idl/OBJ-$(ARCH)-$(L4API),$(IDL_PKGDIR))
ifneq ($(IDL_TYPE), dice)
ifneq ($(IDL_TYPE), corba)
$(error IDL_TYPE "$(IDL_TYPE)" not supported.)
endif
endif

# A function to deliver the source files generated from IDL-files and
# that should be linked to a given target. Used by "targetobjs" to write
# the target-objects and in the "Makefile.inc:"-rule to write the IDL_SRC_C-
# variables
# arg: 1 - target
IDL_SRC_Cfn = $(CLIENTIDL:.idl=-client.c) $(SERVERIDL:.idl=-server.c)	\
	      $(foreach inter, $(OSYSTEM) $(1) $(1)_$(OSYSTEM),		\
			$(CLIENTIDL_$(inter):.idl=-client.c)		\
			$(SERVERIDL_$(inter):.idl=-server.c))
IDL_SRC_CCfn = $(CLIENTIDL:.idl=-client.cc) $(SERVERIDL:.idl=-server.cc) \
	      $(foreach inter, $(OSYSTEM) $(1) $(1)_$(OSYSTEM),		\
			$(CLIENTIDL_$(inter):.idl=-client.cc)		\
			$(SERVERIDL_$(inter):.idl=-server.cc))

# *all* CLIENTIDLs/SERVERIDLs
# needed for the test if we should include the IDL-Path into vpath
IDL_ALLCLIENTIDL = $(CLIENTIDL) $(CLIENTIDL_$(OSYSTEM))			\
		   $(foreach target,$(TARGET) $(TARGET_$(OSYSTEM)),	\
		      $(CLIENTIDL_$(target)) $(CLIENTIDL_$(target)_$(OSYSTEM)))
IDL_ALLSERVERIDL = $(SERVERIDL) $(SERVERIDL_$(OSYSTEM)) 		\
		   $(foreach target,$(TARGET) $(TARGET_$(OSYSTEM)),	\
		      $(SERVERIDL_$(target)) $(SERVERIDL_$(target)_$(OSYSTEM)))

ifneq ($(IDL_ALLCLIENTIDL),)
vpath %-client.c $(IDL_PATH)
vpath %-client.cc $(IDL_PATH)
endif
ifneq ($(IDL_ALLSERVERIDL),)
vpath %-server.c $(IDL_PATH)
vpath %-server.cc $(IDL_PATH)
endif


# a helper function to generate the object-files for a given target
# arg: 1 - target
#      2 - infix between the basename and the .o
targetobjs = $(SRC_C:.c=$(2).o) $(SRC_S:.S=$(2).o)                     	\
	     $(patsubst %.cc,%$(2).o,$(patsubst %.cpp,%.cc,$(SRC_CC)))	\
	     $(foreach inter, $(OSYSTEM) $(1) $(1)_$(OSYSTEM),		\
			$(SRC_C_$(inter):.c=$(2).o)			\
			$(SRC_S_$(inter):.S=$(2).o)			\
			$(patsubst %.cc,%$(2).o,$(SRC_CC_$(inter):.cpp=.cc)) \
			$($(SRC_CC_$(inter):.cpp=.cc):%.cc=$(2).o))	\
	     $(patsubst %.c,%$(2).o,$(call IDL_SRC_Cfn,$(1)))

# The directories our object files will go into. These dirs will be created
# on generation of Makefile.inc in the OBJ- dirs.
OBJDIRS = $(strip $(sort $(filter-out ./,$(dir 				\
		$(foreach t,$(TARGET_STANDARD),				\
			$(call targetobjs,$(t),))			\
		$(foreach t,$(TARGET_PIC) $(TARGET_SHARED),		\
			$(call targetobjs,$(t),.s))			\
		$(foreach t,$(PROFILE),					\
			$(call targetobjs,$(t),.pr))			\
		$(foreach t,$(TARGET_PROFILE_PIC) $(TARGET_PROFILE_SHARED),\
		$(call targetobjs,$(t),.pr.s))))))

# The include file for the Makefile. This file contains definitions and
# dependencies which are dynamically generated. Unfortunately, make does
# not support variable declarations using the foreach function. Same
# for rules.
$(BID_OBJ_Makefile).inc: .general.d
	@$(BUILD_MESSAGE)
	@$(ECHO) "# automatically generated, do not edit!" >$@_
        # write helper variables containing the IDL-C-Sources
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\nIDL_SRC_C_$(target)=$(call IDL_SRC_Cfn,$(target))))'>>$@_
        # and add them to SRC_C
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\nSRC_C_$(target) += $$(IDL_SRC_C_$(target))))'>>$@_
        # but set the variables empty for the current rule
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\n$(BID_OBJ_Makefile).inc: IDL_SRC_C_$(target)=))'>>$@_
        # write helper variables containing the IDL-C++-Sources
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\nIDL_SRC_CC_$(target)=$(call IDL_SRC_CCfn,$(target))))'>>$@_
        # and add them to SRC_CC
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\nSRC_CC_$(target) += $$(IDL_SRC_CC_$(target))))'>>$@_
        # but set the variables empty for the current rule
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\n$(BID_OBJ_Makefile).inc: IDL_SRC_CC_$(target)=))'>>$@_
        # write the object declaration variables
        # TARGET_{STANDARD, PIC, SHARED} contain the appropriate targets
        # TARGET_PROFILE_{STANDARD, PIC, SHARED} as well
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET_STANDARD),	\
		\nOBJS_$(target) += $(call targetobjs,$(target),)))' >>$@_
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET_PIC)		\
					      $(TARGET_SHARED),		\
		\nOBJS_$(target) += $(call targetobjs,$(target),.s)))' >>$@_
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET_PROFILE),	\
		\nOBJS_$(target) += $(call targetobjs,$(target),.pr)))' >>$@_
	@$(ECHO) -e '$(strip $(foreach target,$(TARGET_PROFILE_PIC)	\
				$(TARGET_PROFILE_SHARED),		\
		\nOBJS_$(target) += $(call targetobjs,$(target),.pr.s)))' >>$@_
        #
        # write the dpe-dependencies
	@$(ECHO) -e '$(foreach obj,$(sort 				\
		$(foreach t,$(TARGET_STANDARD),$(call targetobjs,$t))	\
		$(foreach t,$(TARGET_PIC) $(TARGET_SHARED),		\
			$(call targetobjs,$t,.s))			\
		$(foreach t,$(TARGET_PROFILE),$(call targetobjs,$t,.pr)) \
		$(foreach t,$(TARGET_PROFILE_PIC) $(TARGET_PROFILE_SHARED),\
			$(call targetobjs,$t,.pr.s))),$(strip		\
		$(addprefix \n$(obj): ,$(patsubst %.dpe,%.dpi,		\
		$(DPE_$(obj:.o=.c)) $(DPE_$(obj:.o=.cc))		\
		$(DPE_$(obj:.s.o=.c)) $(DPE_$(obj:.s.o=.cc))		\
		$(DPE_$(obj:.pr.o=.c)) $(DPE_$(obj:.pr.o=.cc))		\
		$(DPE_$(obj:.pr.s.o=.c)) $(DPE_$(obj:.pr.s.o=.cc))	\
		))))' >>$@_
        #
        # create the object-dirs
	@$(if $(OBJDIRS), $(VERBOSE)set -e; for d in $(OBJDIRS); do	\
		[ -d "$$d" ] || $(MKDIR) $$d; done )

        #
        # write the object-dependencies for the targets
	@$(ECHO) -e '$(foreach target,$(TARGET),$(strip\
	          \n$(target): $$(OBJS_$(target)) ))' >> $@_
        # make the target dependent on '.general.d'
	@$(ECHO) -e '$(foreach target,$(TARGET),$(strip			\
	          \n$$(OBJS_$(target)): .general.d))'			\
		>> $@_
        # Using LDSO does not need a RELOC address, so omit the following in
        # this case
ifneq ($(MODE),loader)
        # in case of a binary, write the link address
        # the dependency to the static file: If DEFAULT_RELOC is set,
        # all targets are made dependent on the STATIC file
        # (defined in Makeconf).
        # If DEFAULT_RELOC_xxx is set, xxx is made dependent on the
        # STATIC file.
ifneq ($(strip $(foreach target,$(TARGET), $(call default_reloc,$(target)))),)
ifneq ($(filter l4linux host,$(MODE)),)
	$(error DEFAULT_RELOC must not be set if MODE is l4linux or host)
endif
        # the relocatable binaries depend on Makefile.inc
	@$(ECHO) -e '\n$(strip						 \
		$(foreach target, $(TARGET),				 \
			$(if $(call default_reloc,$(target)),$(target))) \
		: $@)' >>$@_
ifneq ($(wildcard $(STATICFILE)),)
        # Makefile.inc depends on STATIC
	@$(ECHO) -e '\n$@: $(firstword $(wildcard $(STATICFILE)))' >>$@_
        # we find out the relocation address here.
	@$(ECHO) -e '$(foreach target, $(TARGET),$(strip		\
		\nLDFLAGS_$(target)+= -Ttext=$(firstword		\
			$(shell $(AWKP) -v prog=$(target)		\
				"/^[^\#]/{if(\$$2==prog){print $$1; exit}}" \
				< $(firstword $(wildcard $(STATICFILE)))) \
			$(call default_reloc,$(target)))))'	\
		>>$@_
        # and the dummy-rule to rebuild myself if STATIC disappears
	@$(ECHO) -e '\n$(firstword $(wildcard $(STATICFILE))): ' >>$@_
else
        # STATIC does not exist. rebuild myself if STATIC appears
	@$(ECHO) -e '\n$$(if $$(wildcard $(STATICFILE)), $@: FORCE)'	\
		>>$@_
        # we find out the relocation address here.
	@$(ECHO) -e '$(foreach target, $(TARGET),$(strip		\
		\nLDFLAGS_$(target)+= -Ttext=$(firstword		\
			$(call default_reloc,$(target)))))'	\
		>>$@_
endif # STATIC file
endif
endif # MODE != loader
	@mv $@_ $@

ifeq ($(filter scrub help,$(MAKECMDGOALS)),)
-include $(BID_OBJ_Makefile).inc
endif

#
# Rules Section
#

# the default target "all" ensures building of the targets. When multiple
# architectures are used, the targets are build in subdirs.

# the relink-rule: make the TARGETs phony. Specification of build-targets 
# in MAKECMDGOALS is not allowed. Possibility: TARGET=
#
ifneq ($(filter relink,$(MAKECMDGOALS)),)
.PHONY: $(TARGET)
relink: all
endif



###
#
# Compilation rules with dependency-generation
#
# If we have ld.so, we use it to create our dependencies (see MAKEDEP
# definition). Otherwise, we fall back to whatever we need. For
# compilation with gcc, this is using "-MD" and postprocessing the
# generated files.

ifeq ($(HAVE_LDSO),)
LIBDEPS = $(foreach file, \
                    $(patsubst -l%,lib%.a,$(filter-out -L%,$(LDFLAGS))) \
                    $(patsubst -l%,lib%.so,$(filter-out -L%,$(LDFLAGS))),\
                    $(word 1, $(foreach dir, \
                           $(patsubst -L%,%,\
                           $(filter -L%,$(LDFLAGS) $(L4ALL_LIBDIR))),\
                      $(wildcard $(dir)/$(file)))))

DEPEND_EXTEND_CMD = $(AWKP) '			\
/^[^:]*: ..*/{					\
	while(sub("\\\\$$","")){		\
		getline nextline ;		\
		$$0=$$0 " " nextline		\
	}					\
	split($$0,field,": ");			\
	sub("^$(*F).o","$*.s $*.i $@",field[1]);	\
	nr=split(field[2],deps) ;		\
	for(i=1;i<=nr;){			\
	  printf("ifeq ($$(shell test y \\\n");	\
	  for(j=0; j<100 && i+j<=nr; j++){	\
	    printf("-a -r %s \\\n", deps[i+j]);	\
	  }					\
	  printf("&& echo y),)");		\
	  printf("\n%s: FORCE\nelse\n%s: $(BID_OBJ_Makefile) \\\n",	\
	  	field[1],field[1]);		\
	  for(j=0; j<100 && i+j<=nr; j++){	\
	    printf("%s \\\n",deps[i+j]);	\
	  }					\
	  printf("\nendif\n");			\
	  i+=j;					\
	}					\
}'
DEPEND_EXTEND_FUNC = ( $(DEPEND_EXTEND_CMD) < $(1) > $(2).new && rm -f $(1) && mv $(2).new $(2) ) || ( rm -f $(1) $(2).new $(2) ; $(DEPEND_IGNORE_ERRORS) )
DEPEND_FLAG = -MD

.PHONY: FORCE
endif # HAVE_LDSO empty

%.o:	%.cc
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.s.o:	%.cc
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(PICFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.o:	%.cc
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.s.o: %.cc
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(PICFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.o:	%.cpp
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.s.o:	%.cpp
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(PICFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.o:	%.cpp
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.s.o: %.cpp
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CXX_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) $(PICFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.o:	%.c
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.s.o:	%.c
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.o:	%.c
	@$(COMP_PR_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.s.o: %.c
	@$(COMP_PR_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.o:	%.S
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(ASFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.s.o:	%.S
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(ASFLAGS) $(PICFLAGS) $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.o:	%.S
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(ASFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.s.o:	%.S
	@$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(ASFLAGS) $(PICFLAGS) -DPROFILE -pg $(call absfilename,$<) -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.c:	%.y
	@$(GEN_MESSAGE)
	$(VERBOSE)$(YACC) $(YFLAGS) $(call absfilename,$<)
	$(VERBOSE)mv -f y.tab.c $@
	$(VERBOSE)if [ -f y.tab.h ]; then mv -f y.tab.h $(@:.c=.h); fi

%.c:	%.l
	@$(COMP_MESSAGE)
	$(VERBOSE)$(LEX) -o$@ $(call absfilename,$<)

%.i:	%.c
	@$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -E -H -dD $(CPPFLAGS) $(CFLAGS) $(call absfilename,$<) -o $@
	$(VERBOSE)$(INDENT) -o $@ $@

%.s.i:	%.c
	@$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -E -H -dD $(CPPFLAGS) $(CFLAGS) $(PICFLAGS) $(call absfilename,$<) -o $@
	$(VERBOSE)$(INDENT) -o $@ $@

%.i:	%.cc
	@$(COMP_MESSAGE)
	$(VERBOSE)$(CXX) -E -H -dD $(CPPFLAGS) $(CXXFLAGS) $(call absfilename,$<) -o $@
	$(VERBOSE)$(INDENT) -sob -o $@ $@

%.s.i:	%.cc
	@$(COMP_MESSAGE)
	$(VERBOSE)$(CXX) -E -H -dD $(CPPFLAGS) $(CXXFLAGS) $(PICFLAGS) $(call absfilename,$<) -o $@
	$(VERBOSE)$(INDENT) -sob -o $@ $@

%.i:	%.S
	@$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -E -H -dD $(CPPFLAGS) $(ASFLAGS) $(call absfilename,$<) -o $@
	$(VERBOSE)$(INDENT) -sob -o $@ $@

%.S:	%.c
	@$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -S $(CFLAGS) $(CPPFLAGS) $(call absfilename,$<) -o $@

%.dpi:	%.dpe
	@$(GEN_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,perl) $(GEN_DOPECODE) $< >$@

DEPS	+= $(foreach file,$(ALLOBJS), $(dir $(file)).$(notdir $(file)).d)
DEPS	+= $(foreach file,$(ALLDPI), $(dir $(file)).$(notdir $(file)).d)

# Common clean Rules

clean cleanall::
	$(VERBOSE)$(RM) $(strip $(filter-out $(KEEP_ON_CLEAN),\
		$(wildcard *.dpi) $(wildcard *.o) $(ALLOBJS) $(DEL_ON_CLEAN)))

# clean: delete all temporary editor files, objects, binaries
#        and most generated files
cleanall::
	$(VERBOSE)$(RM) $(TARGET) $(wildcard .*.d) $(wildcard $(BID_OBJ_Makefile).inc)

.PHONY: scrub clean cleanall

endif	# $(SYSTEM) is defined

# General rules

# scrub: delete temporary editor files and stuff like this
#        can be extended by lokals
scrub cleanall::
	$(VERBOSE)$(SCRUB)

