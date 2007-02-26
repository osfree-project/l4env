PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

CXXFLAGS_amd64 += -mcmodel=medium
CFLAGS_amd64 += -mcmodel=medium

include $(L4DIR)/mk/Makeconf

SYSTEMS		= x86 arm amd64
TARGET		= libuc_c.a
SYSDEPS		:= libc/sysdeps/linux
VPATH_SRC_BASE  = $(OBJ_DIR)

ifneq ($(SYSTEM),)
UCLIBC_ARCH_x86   := i386
UCLIBC_ARCH_arm	  := arm
UCLIBC_ARCH_amd64 := x86_64
DIRS		:= libc/stdio libc/stdlib libc/stdlib/malloc libc/string \
		   libc/string/$(UCLIBC_ARCH_$(ARCH)) libc/string/generic \
		   libc/inet libc/misc libc/misc/assert libc/misc/ctype \
		   libc/misc/dirent libc/misc/internals libc/misc/locale \
		   libc/misc/time libc/pwd_grp libc/unistd libc/termios \
		   libc/misc/wctype libc/misc/wchar \
		   libc/signal $(SYSDEPS)/$(UCLIBC_ARCH_$(ARCH)) \
		   $(SYSDEPS)/common libm

SRC_C		:= $(foreach arch,all $(ARCH),\
		    $(foreach dir,$(DIRS), \
		     $(addprefix ARCH-$(arch)/$(dir)/,\
		      $(notdir $(wildcard ../ARCH-$(arch)/$(dir)/*.c)))))
SRC_S		:= $(foreach arch,all $(ARCH),\
		    $(foreach dir,$(DIRS), \
		     $(addprefix ARCH-$(arch)/$(dir)/,\
		      $(notdir $(wildcard ../ARCH-$(arch)/$(dir)/*.S)))))

IDL_PATH	=
PRIVATE_INCDIR	= $(SRC_DIR)/ARCH-$(ARCH)/include $(SRC_DIR)/ARCH-all/include \
		  $(OBJ_DIR)/ARCH-$(ARCH)/include $(OBJ_DIR)/ARCH-all/include \
		  $(OBJ_BASE)/include/$(ARCH) $(OBJ_BASE)/include
MODE		= host
LIBCINCDIR	= $(addprefix -I,$(GCCINCDIR))
CPPFLAGS	= -nostdinc -include \
		  $(OBJ_DIR)/ARCH-all/include/libc-symbols.h
CFLAGS		= -fno-builtin
# CFLAGS	+= -std=iso9899:199901
DEFINES		= -DNDEBUG -D_LIBC -DSTATIC
WARNINGS	= -Wall -Wstrict-prototypes

ifneq ($(L4API),)
DEFINES		+= -DL4_THREAD_SAFE
TARGET		= libuc_c_r.a
PRIVATE_INCDIR	= $(SRC_DIR)/ARCH-$(ARCH)/include $(SRC_DIR)/ARCH-all/include \
		  $(OBJ_DIR)/ARCH-$(ARCH)/include $(OBJ_DIR)/ARCH-all/include \
		  $(OBJ_BASE)/include/$(ARCH)/$(L4API) \
		  $(OBJ_BASE)/include/$(ARCH) $(OBJ_BASE)/include
endif

endif

BUILD_ARCHS	:= all $(BUILD_ARCH)

FILES_all	:= $(PKGDIR_ABS)/lib/uclibc/contrib_files_all.lst
FILES_x86	:= $(PKGDIR_ABS)/lib/uclibc/contrib_files_x86.lst
FILES_arm	:= $(PKGDIR_ABS)/lib/uclibc/contrib_files_arm.lst
FILES_amd64     := $(PKGDIR_ABS)/lib/uclibc/contrib_files_amd64.lst
CONTRIB_DIR	:= $(PKGDIR_ABS)/lib/contrib/uclibc

.general.d: $(FILES_x86) $(FILE_arm) $(FILES_amd64) $(FILES_all)

ifeq ($(SYSTEM),)

INCDIRS		= $(addsuffix /include,$(addprefix $(OBJ_DIR)/ARCH-,$(BUILD_ARCHS)))
INCDIRS_OBJ_m   = $(addsuffix /Makefile,$(INCDIRS))

all:: include

$(INCDIRS_OBJ_m):
	$(VERBOSE)echo 'PKGDIR   ?= $(PKGDIR_ABS)' > $@
	$(VERBOSE)echo 'L4DIR    ?= $(L4DIR_ABS)' >> $@
	$(VERBOSE)echo 'OBJ_BASE ?= $(OBJ_BASE)' >> $@
	$(VERBOSE)echo 'OBJ_DIR  ?= $(OBJ_DIR)' >> $@
	$(VERBOSE)echo 'INSTALL_INC_PREFIX = $$(PKGNAME)' >> $@
	$(VERBOSE)echo 'include $$(L4DIR)/mk/include.mk' >> $@

include:: $(addprefix $(OBJ_DIR)/ARCH-,$(addsuffix /links-done,$(BUILD_ARCHS))) $(INCDIRS_OBJ_m)
	$(VERBOSE)for i in $(INCDIRS); do \
	  PWD=$(PWD)/$$i $(MAKE) -C $$i all; \
	done

install:: all
	$(VERBOSE)set -e; $(foreach d,$(INCDIRS), \
	  if [ -f $d/Makefile ]; then \
	    PWD=$(PWD)/$d $(MAKE) -C $d $@ $(MKFLAGS); \
	  fi;)

# ok, we have a list of files to check in FILES
# for each file do
# check if it exists, yes? go on
#   no? create an apropriate link with a relative path to the contrib dir

REBUILD_LINKS_CONTRIB =                                                  \
  set -e; arch=$1;                                                       \
    cat contrib_files_$${arch}.lst | while read file flags;              \
    do                                                                   \
      path_file=`dirname $${file}`;                                      \
      path_ctrb=`dirname $(CONTRIB_DIR)/$${file}`;                       \
      if [ -n "$${flags}" ]; then                                        \
        dest="$${path_file}/$${flags}.c";                                \
      else                                                               \
        dest="$${file}";                                                 \
      fi;                                                                \
      if [ "$${path_file}" = "$(SYSDEPS)/i386/bits" -o                   \
           "$${path_file}" = "$(SYSDEPS)/arm/bits"  -o                   \
           "$${path_file}" = "$(SYSDEPS)/x86_64/bits"  -o                \
           "$${path_file}" = "$(SYSDEPS)/common/bits" ]; then            \
        if [ \( ! -e ARCH-$${arch}/include/bits/`basename $${dest}` \)   \
                  -a \( -e $(CONTRIB_DIR)/$${file} \) ]; then            \
	  b=`basename $${file}`;                                         \
          from=$${path_ctrb}/$$b;                                        \
          to=$(OBJ_DIR)/ARCH-$${arch}/include/bits/$$b;                  \
        fi;                                                              \
      elif [ "$${path_file}" = "$(SYSDEPS)/i386/sys" -o                  \
             "$${path_file}" = "$(SYSDEPS)/arm/sys"  -o                  \
             "$${path_file}" = "$(SYSDEPS)/x86_64/sys"  -o               \
             "$${path_file}" = "$(SYSDEPS)/common/sys" ]; then           \
        if [ \( ! -e ARCH-$${arch}/include/sys/`basename $${dest}` \)    \
                  -a \( -e $(CONTRIB_DIR)/$${file} \) ]; then            \
	  b=`basename $${file}`;                                         \
          from=$${path_ctrb}/$$b;                                        \
          to=$(OBJ_DIR)/ARCH-$${arch}/include/sys/$$b;                   \
        fi;                                                              \
      else                                                               \
        if [ -e $(CONTRIB_DIR)/$${file} ] ; then                         \
          from=$${path_ctrb}/`basename $${file}`;                        \
          to=$(OBJ_DIR)/ARCH-$${arch}/$${dest};                          \
        fi;                                                              \
      fi;                                                                \
      if [ -n "$$to" -a -n "$$from" -a ! -e "$$to" ]; then               \
        d=`dirname $$to`;                                                \
        [ -d "$$d" ] || mkdir -p $$d;                                    \
        ln -s$(if $(VERBOSE),,v) "$$from" "$$to";                        \
      fi;                                                                \
      unset to; unset from;                                              \
    done;

REBUILD_LINKS_SRC =                                                      \
  set -e; arch=$1;                                                       \
    find ARCH-$$arch -name \*.h -o -name \*.c -o -name \*.S |            \
    while read file;                                                     \
    do                                                                   \
      to=$(OBJ_DIR)/$$file;                                              \
      d=$$(dirname $$to);                                                \
      [ -d "$$d" ] || mkdir -p $$d;                                      \
      ln -s$(if $(VERBOSE),,v) "$(SRC_DIR)/$$file" "$$to";                          \
    done;


$(OBJ_DIR)/ARCH-%/links-done: contrib_files_%.lst
	@echo -n "  ... Checking and fixing contrib file links, cleaning .o [$*]"
	$(VERBOSE)if [ -d "$(OBJ_DIR)/ARCH-$*" ]; then                      \
	  find $(OBJ_DIR)/ARCH-$* -type l | xargs rm -f$(if $(VERBOSE),,v); \
	fi
	$(VERBOSE)$(call REBUILD_LINKS_SRC,$*)
	$(VERBOSE)$(call REBUILD_LINKS_CONTRIB,$*)
	@touch $@
	@echo " ... done!"

else

CONTRIB: $(addprefix $(OBJ_DIR)/ARCH-,$(addsuffix /links-done,$(BUILD_ARCHS)))

endif

cleanall::
	$(VERBOSE)find $(OBJ_DIR) -type l | xargs rm -f$(if $(VERBOSE),,v)
	$(VERBOSE)$(RM) $(OBJ_DIR)/ARCH-*/links-done
	$(VERBOSE)$(RM) $(OBJ_DIR)/ARCH-*/include/.general.d

# some special defines
O_AND_SO_OBJS	= $(SRC_C:.c=.o) $(SRC_C:.c=.s.o)
$(filter %__DO_UNLOCKED.o %__DO_UNLOCKED.s.o,$(O_AND_SO_OBJS)):\
	DEFINES += -D__DO_UNLOCKED
$(filter %__DO_LARGEFILE.o %__DO_LARGEFILE.s.o,$(O_AND_SO_OBJS)):\
	DEFINES += -D__DO_LARGEFILE
$(filter %__FN.o,$(SRC_C:.c=.o)):\
	DEFINES += -DL_$(patsubst %__FN.o,%,$(notdir $@))
$(filter %__FN.s.o,$(SRC_C:.c=.s.o)):\
	DEFINES += -DL_$(patsubst %__FN.s.o,%,$(notdir $@))

$(SRC_C_libm:.c=.o) $(SRC_C_libm:.c=.s.o):\
	DEFINES += -D_IEEE_LIBM -D_ISOC99_SOURCE -D_SVID_SOURCE

include $(L4DIR)/mk/lib.mk
