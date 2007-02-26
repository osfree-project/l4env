# -*- Makefile -*-
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

.general.d:	$(L4DIR)/mk/binary.inc


ifeq ($(SYSTEM),) # if we have no system yet, build the subdirs
#################################################################
#
# we have NO system defined in $(SYSTEM), we are in the src/ dir
#
#################################################################

# our default system
SYSTEMS	?= x86-l4v2

# intersection with BUILD_SYSTEMS
# fist, enure BUILD_SYSTEMS is set
BUILD_SYSTEMS ?= x86_586-l4v2

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

# print that system of the systems variable that actually matched with
# $(BUILD_SYSTEMS) to the given system
# args: $(1) - build architecture (SYSTEM)
#       $(2) - SYSTEMS
# + do nearly the same as in FILTER_SYSTEM, but additionally check if
#   the resulting system matches $(1). If so, print the according pattern from
#   SYSTEMS and exit
ORIG_SYSTEM = $(shell echo $(2)|$(AWKP) '\
		    BEGIN{m=s="$(1)";sub("_[^-]*","",m)}\
		    {for(i=1;i<=NF;i++){\
		 	if(m==$$i||s==$$i){if(s=="$(1)"){print $$i;exit}};\
		 	if(index(m,$$i)==1||index(s,$$i)==1)\
			    {t=s;sub("-.*","",t);\
                             if(t=="$(1)"){print $$i;exit}}}}')

TARGET_SYSTEMS = $(sort $(foreach sys,$(BUILD_SYSTEMS),\
			$(call FILTER_SYSTEM,$(sys),$(SYSTEMS))))

DIR_FROM_SUB = $(firstword $(addprefix ../,$(patsubst /%,,$(1))) $(1))
SYSTEM_TO_ARCH = $(shell echo $(1)|$(SED) -e 's/[_-].*//')
SYSTEM_TO_CPU = $(shell echo $(1)|$(SED) -ne 's/[^-_]*_\([^-]*\).*/\1/p')
SYSTEM_TO_L4API = $(shell echo $(1)|$(SED) -ne 's/.*-\(.*\)/\1/p')

$(foreach sys,$(TARGET_SYSTEMS),OBJ-$(sys)/Makefile):OBJ-%/Makefile:.general.d
	$(VERBOSE)install -d $(dir $@)
	$(VERBOSE)echo 'L4DIR=$(call DIR_FROM_SUB,$(L4DIR))'>$@
	$(VERBOSE)echo 'PKGDIR=$(call DIR_FROM_SUB,$(PKGDIR))'>>$@
	$(VERBOSE)echo 'MAKECONFLOCAL=../Makeconf.local'>>$@
	$(VERBOSE)echo 'OSYSTEM=$(call ORIG_SYSTEM,$*,$(SYSTEMS))'>>$@
	$(VERBOSE)echo 'SYSTEM=$*'>>$@
	$(VERBOSE)echo 'ARCH=$(call SYSTEM_TO_ARCH,$*)'>>$@
	$(VERBOSE)echo 'CPU=$(call SYSTEM_TO_CPU,$*)'>>$@
	$(VERBOSE)echo 'L4API=$(call SYSTEM_TO_L4API,$*)'>>$@
	$(VERBOSE)echo 'vpath %.c ..'>>$@
	$(VERBOSE)echo 'vpath %.cc ..'>>$@
	$(VERBOSE)echo 'vpath %.S ..'>>$@
	$(VERBOSE)echo 'vpath %.y ..'>>$@
	$(VERBOSE)echo 'vpath %.l ..'>>$@
	$(VERBOSE)echo 'vpath %.ld ..'>>$@
	$(VERBOSE)echo '.general.d: ../$(if $(wildcard Make.rules),Make.rules,Makefile)'>>$@
	$(VERBOSE)echo 'include ../$(if $(wildcard Make.rules),Make.rules,Makefile)'>>$@
	$(VERBOSE)echo 'include $$(L4DIR)/mk/$(ROLE)'>>$@

all install relink reloc scrub:: $(foreach arch,$(TARGET_SYSTEMS),\
                                          OBJ-$(arch)/Makefile)
	$(if $(TARGET_SYSTEMS),  $(VERBOSE)set -e ; \
	    for d in $(TARGET_SYSTEMS) ; do $(MAKE) -C OBJ-$$d $@ ; done )

clean::
	$(if $(wildcard OBJ-*), $(VERBOSE)set -e ; \
	    for d in $(wildcard OBJ-*) ; do $(MAKE) -C $$d $@ ; done )

cleanall::
	$(VERBOSE)$(RM) -r $(wildcard OBJ-* .general.d)

.PHONY: $(TARGET_SYSTEMS)

else
###############################################################
#
# we have a system defined in $(SYSTEM), we are in an OBJ- dir
#
###############################################################

all::		$(TARGET)

L4INCDIR ?=	$(addprefix $(L4DIR)/include/$(ARCH)/,$(L4API)) \
                $(addprefix $(DROPS_STDDIR)/include/$(ARCH)/,$(L4API)) \
                $(L4DIR)/include/$(ARCH) \
                $(DROPS_STDDIR)/include/$(ARCH) \
                $(L4DIR)/include \
                $(DROPS_STDDIR)/include
L4LIBDIR ?=	$(addprefix $(L4DIR)/lib/$(ARCH)_$(CPU)/,$(L4API)) \
                $(addprefix $(DROPS_STDDIR)/lib/$(ARCH)_$(CPU)/,$(L4API)) \
                $(L4DIR)/lib/$(ARCH)_$(CPU) \
                $(DROPS_STDDIR)/lib/$(ARCH)_$(CPU) \
                $(L4DIR)/lib \
                $(DROPS_STDDIR)/lib

L4LINUXDIR ?=	$(L4DIR)/../linux22

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

# we use these variables to define compiler-switches depending on the system
# we compile for. These variables are evaluated using the target-dependent
# variables ARCH and CPU
CARCHFLAGS_x86_586=-march=i586
CARCHFLAGS_x86_686=-march=i686
CARCHFLAGS_x86_K6=-march=k6
CARCHFLAGS_x86_K7=-march=K7

BID_SUPPORTED_x86_sigma0 = y
LIBCINCDIR_x86_sigma0	= -nostdinc $(addprefix -I,\
		$(L4DIR)/../oskit $(L4DIR)/../oskit/flux/c \
		$(DROPS_STDDIR)/include $(DROPS_STDDIR)/include/flux/c)
LIBCLIBDIR_x86_sigma0	= -L$(L4DIR)/../oskit/lib
LIBCLIBS_x86_sigma0	= -nostdlib -lmc -loskit_support -lkern -llmm -lmc
L4LIBS_x86_sigma0	= -static -llogserver -lnames -ll4util -lrmgr $(GCCLIB)
CRT0_x86_sigma0		= $(call findfile,crt0_getopt.o,$(L4LIBDIR))
CRTN_x86_sigma0		=
LDSCRIPT_x86_sigma0	= $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_x86_host	= y
LIBCINCDIR_x86_host	=
LIBCLIBDIR_x86_host	=
LIBCLIBS_x86_host	=
L4LIBS_x86_host		=
CRT0_x86_host		=
CRTN_x86_host		=
LDSCRIPT_x86_host	=

BID_SUPPORTED_x86_l4linux = y
LIBCINCDIR_x86_l4linux	=
LIBCLIBDIR_x86_l4linux	= -L$(DROPS_STDDIR)/lib
LIBCLIBS_x86_l4linux	= -ldm_generic -ldm_mem -lnames -ll4util \
			  -lrmgr -lloaderif -ll4env -lslab
CRT0_x86_l4linux	=
CRTN_x86_l4linux	=
LDSCRIPT_x86_l4linux	=

BID_SUPPORTED_x86_l4linux_kern = y
LIBCINCDIR_x86_l4linux_kern = -nostdinc -I$(L4LINUXDIR)/include \
			      -D__KERNEL__ -DLINUX_ON_L4
LIBCLIBDIR_x86_l4linux_kern =
LIBCLIBS_x86_l4linux_kern =
CRT0_x86_l4linux_kern	=
CRTN_x86_l4linux_kern	=
LDSCRIPT_x86_l4linux_kern =

BID_SUPPORTED_x86_oskit10_sigma0 = y
LIBCINCDIR_x86_oskit10_sigma0 = -nostdinc $(addprefix -I, \
				 $(L4DIR)/../oskit10 \
				 $(L4DIR)/../oskit10/oskit/c \
				 $(DROPS_STDDIR)/include \
				 $(DROPS_STDDIR)/include/oskit/c)
LIBCLIBDIR_x86_oskit10_sigma0 = -L$(L4DIR)/../oskit10/lib \
				-L$(L4DIR)/$(DROPS_STDDIR)/lib
LIBCLIBS_x86_oskit10_sigma0   = -nostdlib -loskit10_support -loskit_startup \
				-loskit_clientos -loskit_bootp \
				-loskit_linux_fs -loskit_diskpart \
				-loskit_linux_dev -loskit_freebsd_net \
				-loskit10_support -loskit_kern -loskit_lmm \
				-loskit_amm -loskit_c -loskit_kern -lrmgr \
				-loskit_c
L4LIBS_x86_oskit10_sigma0 = -static -llogserver -lnames -ll4util -lrmgr $(GCCLIB)
CRT0_x86_oskit10_sigma0	= $(call findfile,crt0_oskit10_support.o,$(L4LIBDIR))
CRTN_x86_oskit10_sigma0	= $(call findfile,crtn.o,$(L4LIBDIR))
LDSCRIPT_x86_oskit10_sigma0 = $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_x86_l4env	= y
LIBCINCDIR_x86_l4env	= -nostdinc -DOSKIT $(addprefix -I, \
			   $(L4DIR)/../oskit10 \
			   $(L4DIR)/../oskit10/oskit/c \
	                   $(DROPS_STDDIR)/include \
			   $(DROPS_STDDIR)/include/oskit/c)
LIBCLIBDIR_x86_l4env	= -L$(L4DIR)/../oskit10/lib \
			  -L$(DROPS_STDDIR)/lib
LIBCLIBS_x86_l4env	= -nostdlib \
			  -loskit10_support_l4env -ll4env -ll4rm \
			  -ldm_generic -ldm_mem -lthread -loskit_c \
			  -loskit_kern -loskit_clientos -loskit_lmm \
			  -loskit10_support_l4env -loskit_kern \
			  -loskit_c -loskit10_support_l4env
L4LIBS_x86_l4env	= -static -ll4env -ll4rm -ldm_generic -ldm_mem \
			  -lthread -lsemaphore -llogserver -lnames -ll4util \
			  -lrmgr -loskit10_support_l4env -ll4env -ll4rm \
			  -ldm_generic -ldm_mem -lthread -lslab
CRT0_x86_l4env		= $(call findfile,crt0_l4env.o,$(L4LIBDIR))
CRTN_x86_l4env		=
LDSCRIPT_x86_l4env	= $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_x86_l4env_freebsd = y
LIBCINCDIR_x86_l4env_freebsd = -nostdinc -DOSKIT $(addprefix -I, \
			   $(L4DIR)/../oskit10/freebsd/libc/include \
			   $(L4DIR)/../oskit10/freebsd/3.x/src/include \
			   $(L4DIR)/../oskit10/freebsd/3.x/src/sys \
			   $(L4DIR)/../oskit10 \
			   $(L4DIR)/include/g++-3 $(L4DIR)/include/drops \
			   $(DROPS_STDDIR)/include/g++-3 \
			   $(DROPS_STDDIR)/include/drops \
	                   $(DROPS_STDDIR)/include \
			   $(DROPS_STDDIR)/include/oskit/freebsd)
LIBCLIBDIR_x86_l4env_freebsd = -L$(L4DIR)/../oskit10/lib -L$(DROPS_STDDIR)/lib
LIBCLIBS_x86_l4env_freebsd = -nostdlib \
			  -loskit10_support_l4env_full -loskit_startup \
			  -loskit_clientos -loskit_bootp -loskit_linux_fs \
			  -loskit_diskpart -loskit_linux_dev \
			  -loskit_freebsd_net -loskit_kern -loskit_lmm \
			  -loskit_amm -loskit_freebsd_c -loskit_com \
			  -loskit_freebsd_c
L4LIBS_x86_l4env_freebsd = -static -ll4env_oskit10 -ll4rm -ldm_generic \
			  -ldm_mem -lthread -lsemaphore -llogserver -lnames \
			  -ll4util -lrmgr -loskit10_support_l4env_full \
			  -ll4env_oskit10 -ll4rm -ldm_generic -ldm_mem \
			  -lthread -lslab $(GCCLIB)
CRT0_x86_l4env_freebsd	= $(call findfile,crt0_l4env_freebsd.o,$(L4LIBDIR)) \
			  $(call findfile,clientos_init.o,$(L4LIBDIR))
CRTN_x86_l4env_freebsd	= $(call findfile,crtn.o,$(L4LIBDIR))
LDSCRIPT_x86_l4env_freebsd = $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_x86_loader = y
LIBCINCDIR_x86_loader	= -nostdinc -DOSKIT $(addprefix -I, \
			   $(L4DIR)/../oskit10 \
			   $(L4DIR)/../oskit10/oskit/c \
	                   $(DROPS_STDDIR)/include \
			   $(DROPS_STDDIR)/include/oskit/c)
LIBCLIBDIR_x86_loader	= -L$(DROPS_STDDIR)/lib
LIBCLIBS_x86_loader	= -nostdlib -loskit10_support \
			  -loskit10_support_l4env -ll4env -ll4rm \
			  -ldm_generic -ldm_mem -lthread -lslab -loskit_c \
			  -loskit_kern -loskit_clientos -loskit_lmm \
			  -loskit10_support_l4env -loskit_c \
			  -loskit_kern -loskit10_support_l4env
L4LIBS_x86_loader	= -lloader.s
CRT0_x86_loader		= $(call findfile,crt0_l4env_dyn.o,$(L4LIBDIR))
CRTN_x86_loader		=
LDSCRIPT_x86_loader	= $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_ia64_sigma0= y
LIBCINCDIR_ia64_sigma0	= -nostdinc $(addsuffix l4/libc/, $(L4INCDIR))
LIBCLIBDIR_ia64_sigma0	= 
LIBCLIBS_ia64_sigma0	= -nostdlib -lc
L4LIBS_ia64_sigma0	= -static -lnames -ll4util -lrmgr 
CRT0_ia64_sigma0	= $(call findfile,crt0_main.o,$(L4LIBDIR))
CRTN_ia64_sigma0	=
LDSCRIPT_ia64_sigma0	= $(call findfile,main.ld,$(L4LIBDIR))

BID_SUPPORTED ?= $(BID_SUPPORTED_$(ARCH)_$(MODE))
ifneq ($(BID_SUPPORTED),y)
$(error Mode "$(MODE)" is not supported for CPU architecture "$(ARCH)")
endif

LIBCINCDIR ?= $(LIBCINCDIR_$(ARCH)_$(MODE))
LIBCLIBDIR ?= $(LIBCLIBDIR_$(ARCH)_$(MODE))
LIBCLIBS   ?= $(LIBCLIBS_$(ARCH)_$(MODE))
L4LIBS     ?= $(L4LIBS_$(ARCH)_$(MODE))
CRT0	   ?= $(CRT0_$(ARCH)_$(MODE))
CRTN	   ?= $(CRTN_$(ARCH)_$(MODE))
LDSCRIPT   ?= $(LDSCRIPT_$(ARCH)_$(MODE))

# we do not need to put our FLAG-definitions into a special file if we can
# use the target to parameterize it and if it needs not to appear in the deps 
#CRT0	=  $(word 1, $(CRT0_$@_$(OSYSTEM)) $(CRT0_$@) $(CRT0_$(OSYSTEM)))
#CRTN	=  $(word 1, $(CRTN_$@_$(OSYSTEM)) $(CRTN_$@) $(CRTN_$(OSYSTEM)))
LIBS	+= $(strip $(LIBS_$(OSYSTEM)) $(LIBS_$@) $(LIBS_$@_$(OSYSTEM)))


#LDFLAGS	+= $(strip $(LDFLAGS_$(OSYSTEM)) $(LDFLAGS_$@)  $(LDFLAGS_$@_$(OSYSTEM)))

OPTS	?= -g -O2 $(CARCHFLAGS_$(ARCH)_$(CPU))
WARNINGS?= -Wall -Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations $(WARNINGS_$@)

CFLAGS	+= $(CFLAGS_$(OSYSTEM)) $(CFLAGS_$(<F)) $(CFLAGS_$(<F)_$(OSYSTEM))
CFLAGS	+= $(OPTS) $(WARNINGS)
DEFINES += $(DEFINES_$(OSYSTEM)) $(DEFINES_$(<F)) $(DEFINES_$(<F)_$(OSYSTEM))
DEFINES += -DSYSTEM_$(subst -,_,$(SYSTEM)) -DARCH_$(ARCH) -DCPUTYPE_$(CPU) -DL4API_$(L4API)
ifdef DEBUG
ifeq (,$(filter 0 n N, $(DEBUG)))
DEFINES	+= -DDEBUG
endif
endif

CPPFLAGS+= $(CPPFLAGS_$(OSYSTEM)) $(CPPFLAGS_$(<F)) $(CPPFLAGS_$(<F)_$(OSYSTEM))
CPPFLAGS+= $(DEFINES)
CPPFLAGS+= $(addprefix -I, $(PRIVATE_INCDIR) $(PRIVATE_INCDIR_$(OSYSTEM)) $(PRIVATE_INCDIR_$@) $(PRIVATE_INCDIR_$@_$(OSYSTEM)))
CPPFLAGS+= $(if $(CLIENTIDL)$(SERVERIDL),$(addprefix -I, $(IDL_PATH)))
ifneq ($(MODE),host)
CPPFLAGS+= $(addprefix -I, $(L4INCDIR))
endif
CPPFLAGS+= $(LIBCINCDIR)
CPPFLAGS+= $(addprefix -I, $(GCCINCDIR))

CXXFLAGS+= $(CXXFLAGS_$(OSYSTEM)) $(CXXFLAGS_$(@:.o=.cc)) $(CXXFLAGS_$(@:.o=.cc)_$(OSYSTEM))
CXXFLAGS+= $(OPTS) $(WARNINGS)

ASFLAGS+= $(ASFLAGS_$(OSYSTEM)) $(ASFLAGS_$(@:.o=.S)) $(ASFLAGS_$(@:.o=.S)_$(OSYSTEM))
OBJS	+= $(strip $(OBJS_$(OSYSTEM)) $(OBJS_$@) $(OBJS_$@_$(OSYSTEM)))
ALLOBJS	= $(OBJS) $(foreach target,$(TARGET) $(TARGET_$(OSYSTEM)),\
		$(OBJS_$(target)) $(OBJS_$(target)_$(OSYSTEM)))

# implementation of $(CLIENTIDL) and $(SERVERIDL)
IDL_TYPE ?= dice
IDL_PATH ?= $(PKGDIR)/idl/OBJ-$(SYSTEM)
ifneq ($(IDL_TYPE), dice)
ifneq ($(IDL_TYPE), corba)
ifneq ($(IDL_TYPE), flick)
$(error IDL_TYPE "$(IDL_TYPE)" not supported.)
endif
endif
endif

# A function to deliver the source files generated from IDL-files and
# that should be linked to a given target. Used by "targetobjs" to write
# the target-objects and in the "Makefile.inc:"-rule to write the IDL_SRC_C-
# variables
# arg: 1 - target
IDL_SRC_Cfn = $(CLIENTIDL:.idl=-client.c) $(SERVERIDL:.idl=-server.c)	\
	      $(foreach inter, $(OSYSTEM) $(1) $(1)_$(OSYSTEM),	\
			$(CLIENTIDL_$(inter):.idl=-client.c)		\
			$(SERVERIDL_$(inter):.idl=-server.c))

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
endif
ifneq ($(IDL_ALLSERVERIDL),)
vpath %-server.c $(IDL_PATH)
endif


# a helper function to generate the object-files for a given target
# arg: 1 - target
#      2 - infix between the basename and the .o
targetobjs = $(SRC_C:.c=$(2).o) $(SRC_S:.S=$(2).o) $(SRC_CC:.cc=$(2).o)	\
	     $(foreach inter, $(OSYSTEM) $(1) $(1)_$(OSYSTEM),		\
			$(SRC_C_$(inter):.c=$(2).o)			\
			$(SRC_S_$(inter):.S=$(2).o)			\
			$(SRC_CC_$(inter):.cc=$(2).o))			\
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
Makefile.inc: .general.d
	$(BUILD_MESSAGE)
	@$(ECHO) "# automatically generated, do not edit!" >$@_
        # write helper variables containing the IDL-C-Sources
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\nIDL_SRC_C_$(target)=$(call IDL_SRC_Cfn,$(target))))'>>$@_
        # and add them to SRC_C
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\nSRC_C_$(target) += $$(IDL_SRC_$(target))))'>>$@_
        # but set the variables empty for the current rule
	@$(ECHO) -en '$(strip $(foreach target,$(TARGET),		\
		\nMakefile.inc: IDL_SRC_C_$(target)=))'>>$@_
        # write the object declaration variables
        # TARGET_{STANDARD, PIC, SHARED} contain the appropriate targets
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
        # create the object-dirs
	$(if $(OBJDIRS), $(VERBOSE)set -e; for d in $(OBJDIRS); do	\
		$(MKDIR) $$d; done )
        #
        # write the object-dependencies for the targets
	@$(ECHO) -e '$(foreach target,$(TARGET),$(strip\
	          \n$(target): $$(OBJS_$(target)) ))' >> $@_
        # make the target dependent on '.general.d'
	@$(ECHO) -e '$(foreach target,$(TARGET),$(strip			\
	          \n$$(OBJS_$(target)): .general.d))'			\
		>> $@_
        # in case of a binary, write the link address
        # the dependency to the static file: If DEFAULT_RELOC is set,
        # all targets are made dependent on the STATIC file
        # (defined in Makeconf).
        # If DEFAULT_RELOC_xxx is set, xxx is made dependent on the
        # STATIC file.
ifneq ($(strip $(DEFAULT_RELOC) $(foreach target,$(TARGET),		\
					  $(DEFAULT_RELOC_$(target)))),)
        # the relocatable binaries depend on Makefile.inc
	@$(ECHO) -e '\n$(strip						\
		$(foreach target, $(TARGET),				\
			$(if $(DEFAULT_RELOC),$(target),		\
			     $(if $(DEFAULT_RELOC_$(target)),$(target))))\
		: $@)' >>$@_
ifneq ($(wildcard $(STATICFILE)),)
        # Makefile.inc depends on STATIC
	@$(ECHO) -e '\n$@: $(STATICFILE)' >>$@_
        # we find out the relocation address here.
	@$(ECHO) -e '$(foreach target, $(TARGET),$(strip		\
		\nLDFLAGS_$(target)+= -Ttext=$(firstword		\
			$(shell $(AWKP) -v prog=$(target)		\
				"/^[^\#]/{if(\$$2==prog){print $$1; exit}}" \
				< $(STATICFILE))			\
			$(DEFAULT_RELOC_$(target)) $(DEFAULT_RELOC))))'	\
		>>$@_
        # and the dummy-rule to rebuild myself if STATIC disappears
	@$(ECHO) -e '\n$(STATICFILE): ' >>$@_
else
        # STATIC does not exist. rebuild myself if STATIC appears
	@$(ECHO) -e '\n$$(if $$(wildcard $(STATICFILE)), $@: FORCE)'	\
		>>$@_
        # we find out the relocation address here.
	@$(ECHO) -e '$(foreach target, $(TARGET),$(strip		\
		\nLDFLAGS_$(target)+= -Ttext=$(firstword		\
			$(DEFAULT_RELOC_$(target)) $(DEFAULT_RELOC))))'	\
		>>$@_
endif
endif
	@mv $@_ $@

ifeq ($(filter scrub clean cleanall help,$(MAKECMDGOALS)),)
-include Makefile.inc
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
	sub("^$(*F).o","$*.s $*.E $@",field[1]);	\
	nr=split(field[2],deps) ;		\
	for(i=1;i<=nr;){			\
	  printf("ifeq ($$(shell test y \\\n");	\
	  for(j=0; j<100 && i+j<=nr; j++){	\
	    printf("-a -r %s \\\n", deps[i+j]);	\
	  }					\
	  printf("&& echo y),)");		\
	  printf("\n%s: FORCE\nelse\n%s: Makefile \\\n",	\
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
endif

%.o:	%.cc
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.s.o:	%.cc
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) -fPIC $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.o:	%.cc
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) -DPROFILE -pg $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.s.o: %.cc
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CXX) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CXXFLAGS) -fPIC -DPROFILE -pg $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.o:	%.c
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.s.o:	%.c
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) -fPIC $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.o:	%.c
	$(COMP_PR_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) -DPROFILE -pg $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.s.o: %.c
	$(COMP_PR_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $(CFLAGS) -fPIC -DPROFILE -pg $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.o:	%.S
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.s.o:	%.S
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) -fPIC $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.o:	%.S
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) -DPROFILE -pg $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.pr.s.o:	%.S
	$(COMP_MESSAGE)
	$(VERBOSE)$(call MAKEDEP,$(INT_CPP_NAME)) $(CC) -c $(DEPEND_FLAG) $(CPPFLAGS) -fPIC -DPROFILE -pg $< -o $@
	$(DEPEND_VERBOSE)$(call DEPEND_EXTEND_FUNC, $(*F).d, $(dir $@).$(notdir $@).d)

%.c:	%.y
	$(GEN_MESSAGE)
	$(VERBOSE)$(YACC) $<
	$(VERBOSE)mv -f y.tab.c $@

%.c:	%.l
	$(COMP_MESSAGE)
	$(VERBOSE)$(LEX) -t $< > $@

%.E:	%.c
	$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -E $(CPPFLAGS) $(CFLAGS) $< -o $@
	$(VERBOSE)$(INDENT) -o $@ $@

%.E:	%.cc
	$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -E $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	$(VERBOSE)$(INDENT) -sob -o $@ $@

%.E:	%.S
	$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -E $(CPPFLAGS) $(CXXFLAGS) $< -o $@
	$(VERBOSE)$(INDENT) -sob -o $@ $@

%.S:	%.c
	$(COMP_MESSAGE)
	$(VERBOSE)$(CC) -S $(CFLAGS) $(CPPFLAGS) $< -o $@

DEPS	+= $(foreach file,$(ALLOBJS), $(dir $(file)).$(notdir $(file)).d)

# Common clean Rules

# scrub: delete temporary editor files and stuff like this
#        can be extended by lokals
scrub cleanall::
	$(VERBOSE)$(SCRUB)

clean cleanall::
	$(VERBOSE)$(RM) $(wildcard *.o)

# clean: delete all temporary editor files, objects, binaries
#        and most generated files
cleanall::
	$(VERBOSE)$(RM) $(TARGET) $(wildcard .*.d) $(wildcard Makefile.inc)

.PHONY: scrub clean cleanall

endif	# $(SYSTEM) is defined

