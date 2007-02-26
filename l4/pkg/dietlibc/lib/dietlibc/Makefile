PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

SYSTEMS		?= x86 arm amd64
TARGET		= libdiet_c.a

DIRS		:= lib libm libcompat libcruft libcrypt libregex libshell   \
		   libstdio libugly

ifneq ($(SYSTEM),)

S_DIR_x86	:= i386
S_DIR_ia64	:= ia64
S_DIR_arm	:= arm
S_DIR_amd64	:= amd64
S_DIR		= $(S_DIR_$(ARCH))

# Some functions are implemented in 'c' and in 'asm'. Those being double
# implemented for a specific architecture have a corresponding 'c' file
# somewhere and a 'S' file in $(ARCH). For these cases we should just build
# one of those files.

SRC_S		:= $(addprefix $(S_DIR)/,                                   \
		     $(notdir $(wildcard ../$(S_DIR)/*.S)))
SRC_C		:= $(foreach dir,$(DIRS),$(addprefix $(dir)/,               \
		     $(filter-out $(notdir $(SRC_S:.S=.c)),                 \
		       $(notdir $(wildcard ../$(dir)/*.c)))                 \
		      )                                                     \
		    )

# Sometimes there are double implementations not only in 'c' and 'asm'
# but both are in 'S' files. Here too we want to build only one of them.

SYS_S		+= $(addprefix syscalls.s/,$(filter-out $(notdir $(SRC_S)), \
		     $(notdir $(wildcard ../syscalls.s/*.S))))

SRC_S		+= $(SYS_S)

IDL_PATH	=
PRIVATE_INCDIR	= ../include ..
MODE		= host
LIBCINCDIR	= $(addprefix -I,$(GCCINCDIR))
CPPFLAGS	= -nostdinc

# Threadsafe stuff.
#
# We build an architecture independent version of dietlibc which provides
# fopen(), fgetc(), fputc() & Co. as defined in libstdio - without any 
# locking etc. Additionally we build architecture dependent versions of
# dietlibc which are thread-safe. This means that critical operations are
# secured by L4 semaphores.
ifneq (,$(L4API))
DEFINES		+= -DL4_THREAD_SAFE
TARGET		= libdiet_c_r.a
SRC_C		+= $(shell cd $(PKGDIR)/lib/dietlibc && echo libpthread/*.c)
PRIVATE_INCDIR	+= $(L4DIR)/include $(L4DIR)/include/$(ARCH) \
		   $(L4DIR)/include/$(ARCH)/$(L4API)
endif

endif

FILES		:= $(shell cat $(PKGDIR)/lib/dietlibc/contrib_files.lst)
CONTRIB_DIR	:= $(PKGDIR)/lib/contrib/dietlibc

.general.d: $(PKGDIR)/lib/dietlibc/contrib_files.lst

ifeq ($(SYSTEM),)

INCDIRS	= include

all:: $(INCDIRS)

$(INCDIRS): CONTRIB
	$(VERBOSE)PWD=$(PWD)/$@ $(MAKE) -C $@ all

OBSOLETE = $(filter-out $(FILES),\
	     $(foreach dir,$(DIRS) include,$(addprefix $(dir)/,\
	       $(notdir $(wildcard $(PKGDIR)/lib/dietlibc/$(dir)/*.[chS])))))

# ok, we have a list of files to check in FILES
# for each file do
# check if it exists, yes? go on
#   no? create an apropriate link with a relative path to the contrib dir

.PHONY : CONTRIB
CONTRIB:
	@echo -n "  ... Checking and fixing contrib file links "
	$(VERBOSE)set -e; for file in $(FILES);                         \
	do                                                              \
	  if [ \( ! -e $(PKGDIR)/lib/dietlibc/$${file} \) -a            \
	       \( -e $(CONTRIB_DIR)/$${file} \) ] ; then                \
	    ln -s$(if $(VERBOSE),,v) `$(ABS2REL)                        \
	      $$(dirname $(PKGDIR)/lib/dietlibc/$${file})               \
	      $$(dirname $(CONTRIB_DIR)/$${file})`/`basename $${file}`  \
	      $(PKGDIR)/lib/dietlibc/$${file};                          \
	    $(if $(VERBOSE),(echo -n ,;))                               \
	  fi;                                                           \
	done
	$(VERBOSE)set -e; for file in $(OBSOLETE);                      \
	do                                                              \
	  if [ -L $${file} ]; then                                      \
	    rm $${file}; $(if $(VERBOSE),(echo -n ,;))                  \
	  fi                                                            \
	done
	@echo "  ... done!"

else

CONTRIB:

endif

cleanall::
	$(VERBOSE)find . -type l | xargs rm -f$(if $(VERBOSE),,v)

include $(L4DIR)/mk/lib.mk
