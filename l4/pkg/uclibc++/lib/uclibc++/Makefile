PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

CXXFLAGS_amd64 += -mcmodel=medium
CFLAGS_amd64 += -mcmodel=medium

include $(L4DIR)/mk/Makeconf

SYSTEMS		= x86 arm amd64
TARGET		= libuc_c++.a
VPATH_SRC_BASE	= $(OBJ_DIR)

ifneq ($(SYSTEM),)
HAS_VISIBILITY	:= $(shell if $(CXX) -fvisibility-inlines-hidden -S -o \
		     /dev/null -xc++ /dev/null > /dev/null 2>&1; then \
		     echo "-DGCC_HASCLASSVISIBILITY"; fi)
DIRS		:= src src/abi
SRC_CC		:= $(foreach dir,$(DIRS),$(addprefix ARCH-all/$(dir)/,\
		     $(notdir $(wildcard ../ARCH-all/$(dir)/*.cc))))
PRIVATE_INCDIR	= $(SRC_DIR)/ARCH-all/include \
		  $(OBJ_DIR)/ARCH-all/include \
                  $(OBJ_BASE)/include/$(ARCH)/uclibc \
		  $(OBJ_BASE)/include/uclibc $(OBJ_BASE)/include
MODE		= libc
IDL_PATH	=
LIBCINCDIR	= $(addprefix -I,$(GCCINCDIR))
CPPFLAGS	:= -nostdinc
CXXFLAGS	:= -fno-builtin -nostdinc++ -ansi
DEFINES		:= $(HAS_VISIBILITY)
endif

FILES		:= $(PKGDIR_ABS)/lib/uclibc++/contrib_files.lst
CONTRIB_DIR	:= $(PKGDIR_ABS)/lib/contrib/uclibc++

.general.d: $(FILES)

ifeq ($(SYSTEM),)

pre-obj:: include

$(OBJ_DIR)/ARCH-all/include/Makefile: .general.d
	$(VERBOSE)install -d $(dir $@)
	$(VERBOSE)echo 'PKGDIR   ?= $(PKGDIR_ABS)' > $@
	$(VERBOSE)echo 'L4DIR    ?= $(L4DIR_ABS)' >> $@
	$(VERBOSE)echo 'OBJ_BASE ?= $(OBJ_BASE)' >> $@
	$(VERBOSE)echo 'OBJ_DIR  ?= $(OBJ_DIR)' >> $@
	$(VERBOSE)echo 'INSTALL_INC_PREFIX = $$(PKGNAME)' >> $@
	$(VERBOSE)echo 'TARGET   := $$(filter-out Makefile CVS,$$(wildcard *))' >> $@
	$(VERBOSE)echo 'include $$(L4DIR)/mk/include.mk' >> $@

.PHONY: include

include: CONTRIB $(OBJ_DIR)/ARCH-all/include/Makefile
	$(VERBOSE)PWD=$(PWD)/ARCH-all/include $(MAKE) -C \
	   $(OBJ_DIR)/ARCH-all/include all

install:: all
	$(VERBOSE)set -e; $(foreach d,ARCH-all/include, \
	  if [ -f $d/Makefile ]; then \
	    PWD=$(PWD)/$d $(MAKE) -C $d $@ $(MKFLAGS); \
	  fi;)

# ok, we have a list of files to check in FILES
# for each file do
# check if it exists, yes? go on
#   no? create an apropriate link with a relative path to the contrib dir
# rename *.cpp to *.cc to make BID happy

.PHONY : CONTRIB
CONTRIB:
	@echo -n "  ... Checking and fixing contrib file links "
	$(VERBOSE)set -e ; cat $(FILES) | while read file;                   \
	do                                                                   \
	  if [ `basename $${file} .cpp`.cpp = `basename $${file}` ]; then    \
	    dest=`dirname $${file}`/`basename $${file} .cpp`.cc;             \
	  else                                                               \
	    dest=$${file};                                                   \
	  fi;                                                                \
	  from=$(CONTRIB_DIR)/$${file};                                      \
	  to=$(OBJ_DIR)/ARCH-all/$${dest};                                   \
	  if [ \( ! -e $$to \) -a \( -e $$from \) ]; then                    \
	    d=`dirname $$to`;                                                \
	    [ -d "$$d" ] || mkdir -p $$d;                                    \
	    ln -s$(if $(VERBOSE),,v) $$from $$to;                            \
	    $(if $(VERBOSE),(echo -n ,;))                                    \
	  fi;                                                                \
	done
	@echo "  ... done!"

else

CONTRIB:

endif

cleanall::
	$(VERBOSE)find $(OBJ_DIR) -type l | xargs rm -f$(if $(VERBOSE),,v)
	$(VERBOSE)$(RM) $(OBJ_DIR)/ARCH-all/include/.general.d

include $(L4DIR)/mk/lib.mk
