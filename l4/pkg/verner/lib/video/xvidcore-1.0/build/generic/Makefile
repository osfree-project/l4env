##############################################################################
#
# - Unified Makefile for XviD for *nix environments -
#
# Copyright(C) 2003-2004 Edouard Gomez <ed.gomez@free.fr>
#
#
# Description:
#  This Makefile allows building XviD sources to obtain a shared library
#  and a static library. This Makefile uses variables defined in the
#  platform.inc file. This platform.inc file is usually created by the
#  ./configure script whenever a unix shell is available.
#
# Makefile functional dependencies:
#  - echo
#  - rm (with option -r and -f)
#  - cd
#  - make VPATH support (eg: GNU make, solaris 8 make)
#  - ar
#
# Building output:
#  - C means "_C_ompiling"
#  - A means "_A_ssembling"
#  - I means "_I_nstalling"
#  - D means "creating _D_irectory"
#  - Cl means "_Cl_eaning"
#
# NB: (for mingw32/djgpp users)
#   These 2 environments do not provide a shell by default. So it's impossible
#   to use the configure script to generate a platform.inc file suitable for
#   your machine. You have two choices:
#    - install minsys from the mingw project or install cygwin and then use
#      the configure script as on a unix system.
#    - write a platform.inc file by hand.
#
# PS: default build directory is "=build", it fits naming conventions that
#     make the arch/tla revision control program ignore files contained in
#     this directory during commits operations. This choice is completly
#     arbitrary, but try not to change it.
#
##############################################################################

include sources.inc
include platform.inc

VERBOSE?=@
RM = rm -rf
MKDIR = mkdir -p

##############################################################################
#
# Build rules
#
##############################################################################

# Their Objects
OBJECTS  = $(GENERIC_OBJECTS)
OBJECTS += $(ASSEMBLY_OBJECTS)
OBJECTS += $(DCT_IA64_OBJECTS)

# The VPATH mechanism could use a "per target" build directory
# To keep it simple at the moment, the directory is fixed to "build"
BUILD_DIR = $(OBJ_DIR)/..
VPATH     = $(XVID_SRC_DIR):$(BUILD_DIR)

#-----------------------------------------------------------------------------
# The default rule
#-----------------------------------------------------------------------------

.SUFFIXES: .$(OBJECT_EXTENSION) .$(ASSEMBLY_EXTENSION) .c

all:: $(STATIC_LIB) 
	$(VERBOSE)echo
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo " XviD has been successfully built."
	$(VERBOSE)echo
	$(VERBOSE)echo " * Binaries are currently located in the '$(BUILD_DIR)' directory"
	$(VERBOSE)echo " * To install them on your system, you can run '# make install'"
	$(VERBOSE)echo "   as root."
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo

$(OBJECTS): platform.inc

$(BUILD_DIR):
	$(VERBOSE)echo "  D: $(BUILD_DIR)"
	$(VERBOSE)$(MKDIR) $(BUILD_DIR)

#-----------------------------------------------------------------------------
# Generic assembly rule
#-----------------------------------------------------------------------------

.$(ASSEMBLY_EXTENSION).$(OBJECT_EXTENSION):
	$(VERBOSE)echo "  A: $(@D)/$(<F)"
	$(VERBOSE)$(MKDIR) $(BUILD_DIR)/$(@D)
	$(VERBOSE)$(AS) $(AFLAGS) $< -o $(BUILD_DIR)/$@

#-----------------------------------------------------------------------------
# Generic C rule
#-----------------------------------------------------------------------------

.c.$(OBJECT_EXTENSION):
	$(VERBOSE)echo "  C: $(@D)/$(<F)"
	$(VERBOSE)$(MKDIR) $(BUILD_DIR)/$(@D)
	$(VERBOSE)$(CC) -c $(ARCHITECTURE) $(BUS) $(ENDIANNESS) $(FEATURES) $(SPECIFIC_CFLAGS) $(CFLAGS) $< -o $(BUILD_DIR)/$@

#-----------------------------------------------------------------------------
# Static Library
#-----------------------------------------------------------------------------

$(STATIC_LIB): $(OBJECTS)
	$(VERBOSE)echo "  L: $(@F)"
	$(VERBOSE)cd $(BUILD_DIR) && ar rc $(@F) $(OBJECTS) && $(RANLIB) $(@F)

#-----------------------------------------------------------------------------
# Installation
#-----------------------------------------------------------------------------

install: $(BUILD_DIR)/$(STATIC_LIB) 
	$(VERBOSE)echo " "

#-----------------------------------------------------------------------------
# Platorm specific file -- dumb rules for people executing make before
# ./configure or even ./bootstrap.sh
#-----------------------------------------------------------------------------

platform.inc: configure platform.inc.in
	./configure

configure:
	./bootstrap.sh

#-----------------------------------------------------------------------------
# .PHONY targets
#-----------------------------------------------------------------------------

.PHONY: mrproper distclean clean info \
	list-objects list-targets list-install-path list-cflags

clean:
	$(VERBOSE)echo "  Cl: Build directory"
	$(VERBOSE)$(RM) $(call absfilename,$(BUILD_DIR)/)

cleanall: clean

distclean: clean
	$(VERBOSE)echo "  Cl: Generated build files"
	$(VERBOSE)$(RM) platform.inc
	$(VERBOSE)$(RM) config.log
	$(VERBOSE)$(RM) config.status
	$(VERBOSE)$(RM) autom4te.cache

mrproper: distclean
	$(VERBOSE)echo "  Cl: Boostrapped build files"
	$(VERBOSE)$(RM) configure
	$(VERBOSE)$(RM) install-sh
	$(VERBOSE)$(RM) missing
	$(VERBOSE)$(RM) config.guess
	$(VERBOSE)$(RM) mkinstalldirs
	$(VERBOSE)$(RM) config.sub

list-objects:
	$(VERBOSE)echo
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo "Object files used for this build"
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo
	$(VERBOSE)echo $(OBJECTS)
	$(VERBOSE)echo

list-targets:
	$(VERBOSE)echo
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo "Target Libraries"
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo
	$(VERBOSE)echo Shared library: $(SHARED_LIB)
	$(VERBOSE)echo Static library: $(STATIC_LIB)
	$(VERBOSE)echo

list-install-path:
	$(VERBOSE)echo
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo "Install Paths"
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo
	$(VERBOSE)echo Include Directory: $(includedir)
	$(VERBOSE)echo Library Directory: $(libdir)
	$(VERBOSE)echo

list-cflags:
	$(VERBOSE)echo
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo "Using CFLAGS"
	$(VERBOSE)echo "---------------------------------------------------------------"
	$(VERBOSE)echo
	$(VERBOSE)echo CFLAGS=$(CFLAGS)
	$(VERBOSE)echo

info: list-objects list-cflags list-targets list-install-path
