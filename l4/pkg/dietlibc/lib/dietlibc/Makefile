PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

# the default is to build the listed directories, provided that they
# contain a Makefile. If you need to change this, uncomment the following
# line and adapt it.

#include $(L4DIR)/mk/subdir.mk


S_DIR_x86	= i386
S_DIR_ia64	= ia64
S_DIR_arm	= arm

S_DIR = $(S_DIR_$(ARCH))

# Some functions are implemented in 'c' and in 'asm'. Those being double
# implemented for a specific architecture have a corresponding 'c' file
# somewhere and a 'S' file in $(ARCH). For these cases we should just build
# one of those files.

SRC_S	:= $(addprefix $(S_DIR)/, \
	      $(notdir $(wildcard $(PKGDIR)/lib/dietlibc/$(S_DIR)/*.S)))

dirs	:= lib libm libcruft libcrypt libshell libstdio libugly
SRC_C	:= $(foreach dir,$(dirs),$(addprefix $(dir)/,                     \
	      $(filter-out $(notdir $(SRC_S:.S=.c)),                      \
	         $(notdir $(wildcard $(PKGDIR)/lib/dietlibc/$(dir)/*.c))) \
	       )                                                          \
	    )

# Sometimes there are double implementations not only in 'c' and 'asm'
# but both are in 'S' files. Here too we want to build only one of them.

SYS_S	+= $(addprefix syscalls.s/,$(filter-out $(notdir $(SRC_S)), \
	     $(notdir $(wildcard $(PKGDIR)/lib/dietlibc/syscalls.s/*.S))))

SRC_S	+= $(SYS_S)

.general.d: $(PKGDIR)/lib/dietlibc/contrib_files.lst
FILES	:= $(shell cat $(PKGDIR)/lib/dietlibc/contrib_files.lst)
CONTRIB_DIR := $(PKGDIR)/lib/contrib/dietlibc

PRIVATE_INCDIR	= ../include ..
MODE		= host
LIBCINCDIR	= $(addprefix -I,$(GCCINCDIR))
CPPFLAGS	= -nostdinc

SYSTEMS		= x86 arm
TARGET_x86	= libdiet_c.a
TARGET_arm	= libdiet_c.a
ifeq ($(SYSTEM),)
SUBDIRS	= include

all:: CONTRIB $(SUBDIRS)
$(SUBDIRS): CONTRIB

# ok, we have a list of files to check in FILES
# for each file do
# check if it exists, yes? go on
#   no? create an apropriate link with a relative path to the contrib dir

.PHONY : CONTRIB
CONTRIB:
	@echo -n "  ... Checking and fixing contrib file links "
	$(VERBOSE)set -e ; for file in $(FILES);                        \
	do                                                              \
	  if [ \( ! -e $(PKGDIR)/lib/dietlibc/$${file} \) -a            \
	       \( -e $(CONTRIB_DIR)/$${file} \) ] ; then                \
	    ln -s$(if $(VERBOSE),,v) `$(PKGDIR)/lib/dietlibc/abs2rel.sh \
	      $$(dirname $(PKGDIR)/lib/dietlibc/$${file})               \
	      $$(dirname $(CONTRIB_DIR)/$${file})`/`basename $${file}`  \
	      $(PKGDIR)/lib/dietlibc/$${file};                          \
	    $(if $(VERBOSE),(echo -n ,;))                               \
	  fi;                                                           \
	done
	@echo "  ... done!"
endif

cleanall::
	$(VERBOSE)find . -type l | xargs rm -f$(if $(VERBOSE),,v)

include $(L4DIR)/mk/lib.mk
