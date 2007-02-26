PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

SYSTEMS		= x86 # arm
TARGET		= libuc_c++.a

ifneq ($(SYSTEM),)
HAS_VISIBILITY	:= $(shell if $(CXX) -fvisibility-inlines-hidden -S -o \
		     /dev/null -xc++ /dev/null > /dev/null 2>&1; then \
		     echo "-DGCC_HASCLASSVISIBILITY"; fi)
DIRS		:= src src/abi
SRC_CC		:= $(foreach dir,$(DIRS),$(addprefix ARCH-all/$(dir)/,\
		     $(notdir $(wildcard ../ARCH-all/$(dir)/*.cc))))
PRIVATE_INCDIR	= ../ARCH-all/include $(L4DIR)/include/$(ARCH)/uclibc \
		  $(L4DIR)/include/uclibc $(L4DIR)/include
MODE		= host
IDL_PATH	=
LIBCINCDIR	= $(addprefix -I,$(GCCINCDIR))
CPPFLAGS	:= -nostdinc
CXXFLAGS	:= -fno-builtin -nostdinc++ -ansi
DEFINES		:= $(HAS_VISIBILITY)
endif

FILES		:= $(PKGDIR)/lib/uclibc++/contrib_files.lst
CONTRIB_DIR	:= $(PKGDIR)/lib/contrib/uclibc++

.general.d: $(FILES)

ifeq ($(SYSTEM),)

all:: ARCH-all/include

ARCH-all/include: CONTRIB
	$(VERBOSE)PWD=$(PWD)/$@ $(MAKE) -C $@ all

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
	  if [ \( ! -e ARCH-all/$${dest} \)                                  \
	      -a \( -e $(CONTRIB_DIR)/$${file} \) ] ; then                   \
	    ln -s$(if $(VERBOSE),,v) `$(ABS2REL)                             \
	      ARCH-all/$$(dirname $${file})                                  \
	      $$(dirname $(CONTRIB_DIR)/$${file})`/`basename $${file}`       \
	      ARCH-all/$${dest};                                             \
	    $(if $(VERBOSE),(echo -n ,;))                                    \
	  fi;                                                                \
	done
	@echo "  ... done!"

else

CONTRIB:

endif

cleanall::
	$(VERBOSE)find . -type l | xargs rm -f$(if $(VERBOSE),,v)

include $(L4DIR)/mk/lib.mk
