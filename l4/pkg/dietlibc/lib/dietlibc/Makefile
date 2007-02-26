PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

SYSTEMS		?= x86 arm amd64
TARGET		= libdiet_c.a
VPATH_SRC_BASE  = $(OBJ_DIR)

DIRS		:= lib libm libcompat libcruft libcrypt libregex libshell   \
		   libstdio libugly

# As we differ between 'SYSTEM is empty' and 'SYSTEM is not empty' we don't
# need to fiddle with absolute pathes.

ifneq ($(SYSTEM),)

S_DIR_x86	:= i386
S_DIR_ia64	:= ia64
S_DIR_arm	:= arm
S_DIR_amd64	:= x86_64
S_DIR		= $(S_DIR_$(ARCH))

# Some functions are implemented in 'c' and in 'asm'. Those being double
# implemented for a specific architecture have a corresponding 'c' file
# somewhere and a 'S' file in $(ARCH). For these cases we should just build
# one of those files.

SRC_S		:= $(addprefix $(S_DIR)/,                                    \
		    $(notdir $(wildcard $(OBJ_DIR)/$(S_DIR)/*.S))) aliases.S
SRC_C		:= $(foreach dir,$(DIRS),                                    \
		    $(addprefix $(dir)/,                                     \
		     $(filter-out $(notdir $(SRC_S:.S=.c)),                  \
		      $(notdir $(wildcard $(OBJ_DIR)/$(dir)/*.c)))))

# Sometimes there are double implementations not only in 'c' and 'asm'
# but both are in 'S' files.  Here too we want to build only one of them.

SYS_S		+= $(addprefix $(OBJ_DIR)/syscalls.s/,$(filter-out $(notdir $(SRC_S)), \
		    $(notdir $(wildcard $(OBJ_DIR)/syscalls.s/*.S))))

SRC_S		+= $(SYS_S)

IDL_PATH	=
PRIVATE_INCDIR	= $(OBJ_DIR)/include $(OBJ_DIR) \
		  $(SRC_DIR)/include $(SRC_DIR)
MODE		= host
LIBCINCDIR	= $(addprefix -I,$(GCCINCDIR))
CFLAGS          = -fno-builtin
CPPFLAGS	= -nostdinc

# Threadsafe stuff.
#
# We build an architecture independent version of dietlibc which provides
# fopen(), fgetc(), fputc() & Co. as defined in libstdio - without any 
# locking etc. Additionally we build architecture dependent versions of
# dietlibc which are thread-safe.  This means that critical operations are
# secured by L4 semaphores.
ifneq (,$(L4API))
DEFINES		+= -DL4_THREAD_SAFE
TARGET		= libdiet_c_r.a
SRC_C		+= $(wildcard $(OBJ_DIR)/libpthread/*.c)
PRIVATE_INCDIR	+= $(OBJ_BASE)/include/$(ARCH)/$(L4API) \
		   $(OBJ_BASE)/include/$(ARCH)          \
		   $(OBJ_BASE)/include                  \
		   $(SRC_DIR)/libpthread
		 
endif

endif

CONTRIB_DIR	:= $(PKGDIR_ABS)/lib/contrib/dietlibc
FILES_CONTRIB	:= $(shell cat $(PKGDIR)/lib/dietlibc/contrib_files.lst)
FILES_MODIFIED	:= $(shell echo lib/*.c libstdio/*.c libpthread/*.c)

.general.d: $(PKGDIR)/lib/dietlibc/contrib_files.lst

ifeq ($(SYSTEM),)

INCDIRS	      = include
INCDIRS_OBJ   = $(OBJ_DIR)/include
INCDIRS_OBJ_m = $(INCDIRS_OBJ)/Makefile

all:: $(INCDIRS)

$(INCDIRS_OBJ_m):
	$(VERBOSE)$(MKDIR) $(dir $@)
	$(VERBOSE)echo 'PKGDIR   ?= $(PKGDIR_ABS)' > $@
	$(VERBOSE)echo 'L4DIR    ?= $(L4DIR_ABS)' >> $@
	$(VERBOSE)echo 'OBJ_BASE ?= $(OBJ_BASE)' >> $@
	$(VERBOSE)echo 'OBJ_DIR  ?= $(OBJ_DIR)' >> $@
	$(VERBOSE)echo 'INSTALL_INC_PREFIX = $$(PKGNAME)' >> $@
	$(VERBOSE)echo 'include $$(L4DIR)/mk/include.mk' >> $@

$(INCDIRS): $(INCDIRS_OBJ_m) $(OBJ_DIR)/links-done
	$(VERBOSE)PWD=$(PWD)/$@             $(MAKE) -C $@
	$(VEROBSE)PWD=$(PWD)/$(INCDIRS_OBJ) $(MAKE) -C $(INCDIRS_OBJ)

OBSOLETE = $(filter-out $(FILES_CONTRIB),\
	     $(foreach dir,$(DIRS) include,$(addprefix $(dir)/,\
	       $(notdir $(wildcard $(dir)/*.[chS])))))

# ok, we have a list of files to check in FILES
# for each file do
# check if it exists, yes? go on
#   no? create an apropriate link with a relative path to the contrib dir

REBUILD_LINKS_CONTRIB =                                                 \
  set -e;                                                               \
  for file in $(FILES_CONTRIB);                                         \
  do                                                                    \
    if [ \( ! -e $(OBJ_DIR)/$${file} \) -a                              \
         \( -e $(CONTRIB_DIR)/$${file} \) ] ; then                      \
      d=`dirname $(OBJ_DIR)/$${file}`;                                  \
      [ -d "$$d" ] || mkdir "$$d";                                      \
      ln -s$(if $(VERBOSE),,v)                                          \
        $(CONTRIB_DIR)/$${file} $(OBJ_DIR)/$${file};                    \
      $(if $(VERBOSE),(echo -n ,;))                                     \
    fi;                                                                 \
  done;                                                                 \
  for file in $(FILES_MODIFIED);                                        \
  do                                                                    \
    if [ \( ! -e $(OBJ_DIR)/$${file} \) -a                              \
         \( -e $(SRC_DIR)/$${file} \) ] ; then                          \
      d=`dirname $(OBJ_DIR)/$${file}`;                                  \
      [ -d "$$d" ] || mkdir "$$d";                                      \
      ln -s$(if $(VERBOSE),,v)                                          \
        $(SRC_DIR)/$${file} $(OBJ_DIR)/$${file};                        \
      $(if $(VERBOSE),(echo -n ,;))                                     \
    fi;                                                                 \
  done;                                                                 \
  for file in $(OBSOLETE);                                              \
  do                                                                    \
    if [ -L $${file} ]; then                                            \
      rm $(OBJ_DIR)/$${file}; $(if $(VERBOSE),(echo -n ,;))             \
    fi                                                                  \
  done

REBUILD_LINKS_SRC =                                                     \
  set -e;                                                               \
  find . -name \*.h -o -name \*.c -o -name \*.S | while read file;      \
  do                                                                    \
    to=$(OBJ_DIR)/$$file;                                               \
    d=$$(dirname $$to);                                                 \
    [ -d "$$d" ] || mkdir -p $$d;                                       \
    ln -s$(if $(VERBOSE),,v) "$(SRC_DIR)/$$file" "$$to";           \
  done


$(OBJ_DIR)/links-done: contrib_files.lst
	@echo -n "  ... Checking and fixing contrib file links"
	$(VERBOSE)find $(OBJ_DIR) -type l | xargs rm -f$(if $(VERBOSE),,v)
	$(VERBOSE)$(call REBUILD_LINKS_SRC)
	$(VERBOSE)$(call REBUILD_LINKS_CONTRIB)
	@touch $@
	@echo "  ... done!"

else

CONTRIB: $(OBJ_DIR)/links-done

endif

cleanall::
	$(VERBOSE)find $(OBJ_DIR) -type l | xargs rm -f$(if $(VERBOSE),,v)
	$(VERBOSE)$(RM) $(OBJ_DIR)/links-done

include $(L4DIR)/mk/lib.mk
