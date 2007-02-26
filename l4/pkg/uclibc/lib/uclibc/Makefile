PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

SYSTEMS		= x86 arm
TARGET		= libuc_c.a
SYSDEPS		:= libc/sysdeps/linux

ifneq ($(SYSTEM),)
UCLIBC_ARCH_x86	:= i386
UCLIBC_ARCH_arm	:= arm
DIRS		:= libc/stdio libc/stdlib libc/stdlib/malloc libc/string \
		   libc/string/$(UCLIBC_ARCH_$(ARCH)) libc/string/generic \
		   libc/inet libc/misc libc/misc/assert libc/misc/ctype \
		   libc/misc/dirent libc/misc/internals libc/misc/locale \
		   libc/misc/time libc/pwd_grp libc/unistd libc/termios \
		   libc/signal libc/sysdeps/linux/$(UCLIBC_ARCH_$(ARCH)) \
		   libc/sysdeps/linux/common

SRC_C_libc	:= $(foreach arch,all $(ARCH),\
		     $(foreach dir,$(DIRS),$(addprefix ARCH-$(arch)/$(dir)/,\
		       $(notdir $(wildcard ../ARCH-$(arch)/$(dir)/*.c)))))
SRC_C_libm	:= $(addprefix ARCH-all/libm/,\
		     $(notdir $(wildcard ../ARCH-all/libm/*.c)))
SRC_C		:= $(SRC_C_libc) $(SRC_C_libm)
SRC_S		:= $(foreach arch,all $(ARCH),\
		     $(foreach dir,$(DIRS),$(addprefix ARCH-$(arch)/$(dir)/,\
		       $(notdir $(wildcard ../ARCH-$(arch)/$(dir)/*.S)))))
IDL_PATH	=
PRIVATE_INCDIR	= ../ARCH-$(ARCH)/include ../ARCH-all/include \
		  $(L4DIR)/include/$(ARCH) $(L4DIR)/include
MODE		= host
LIBCINCDIR	= $(addprefix -I,$(GCCINCDIR))
CPPFLAGS	= -nostdinc
CFLAGS		= -fno-builtin
# CFLAGS	+= -std=iso9899:199901
DEFINES		= -DNDEBUG -D_LIBC
WARNINGS	= -Wall -Wstrict-prototypes

ifneq ($(L4API),)
DEFINES		+= -DL4_THREAD_SAFE
TARGET		= libuc_c_r.a
PRIVATE_INCDIR	= ../ARCH-$(ARCH)/include ../ARCH-all/include \
		  $(L4DIR)/include/$(ARCH)/$(L4API) \
		  $(L4DIR)/include/$(ARCH) $(L4DIR)/include
endif

endif

FILES_x86	:= $(PKGDIR)/lib/uclibc/contrib_files_x86.lst
FILES_arm	:= $(PKGDIR)/lib/uclibc/contrib_files_arm.lst
FILES_all	:= $(PKGDIR)/lib/uclibc/contrib_files_all.lst
CONTRIB_DIR	:= $(PKGDIR)/lib/contrib/uclibc

.general.d: $(FILES_x86) $(FILE_arm) $(FILES_all)

ifeq ($(SYSTEM),)

INCDIRS		= $(addsuffix /include,ARCH-all ARCH-x86 ARCH-arm)

all:: $(INCDIRS)

$(INCDIRS): CONTRIB
	$(VERBOSE)PWD=$(PWD)/$@ $(MAKE) -C $@ all

# ok, we have a list of files to check in FILES
# for each file do
# check if it exists, yes? go on
#   no? create an apropriate link with a relative path to the contrib dir

REBUILD_LINKS =                                                          \
  set -e; for arch in all x86 arm;                                       \
  do                                                                     \
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
           "$${path_file}" = "$(SYSDEPS)/common/bits" ]; then            \
        if [ \( ! -e ARCH-$${arch}/include/bits/`basename $${dest}` \)   \
                  -a \( -e $(CONTRIB_DIR)/$${file} \) ]; then            \
          ln -s$(if $(VERBOSE),,v)                                       \
            `$(ABS2REL) ARCH-$${arch}/include/bits                       \
                        $${path_ctrb}`/`basename $${file}`               \
            ARCH-$${arch}/include/bits;                                  \
          $(if $(VERBOSE),(echo -n ,;))                                  \
        fi;                                                              \
      elif [ "$${path_file}" = "$(SYSDEPS)/i386/sys" -o                  \
             "$${path_file}" = "$(SYSDEPS)/arm/sys"  -o                  \
             "$${path_file}" = "$(SYSDEPS)/common/sys" ]; then           \
        if [ \( ! -e ARCH-$${arch}/include/sys/`basename $${dest}` \)    \
                  -a \( -e $(CONTRIB_DIR)/$${file} \) ]; then            \
          ln -s$(if $(VERBOSE),,v)                                       \
            `$(ABS2REL) ARCH-$${arch}/include/sys                        \
                        $${path_ctrb}`/`basename $${file}`               \
            ARCH-$${arch}/include/sys;                                   \
          $(if $(VERBOSE),(echo -n ,;))                                  \
        fi;                                                              \
      else                                                               \
        if [ \( ! -e ARCH-$${arch}/$${dest} \)                           \
            -a \( -e $(CONTRIB_DIR)/$${file} \) ] ; then                 \
          ln -s$(if $(VERBOSE),,v)                                       \
            `$(ABS2REL) ARCH-$${arch}/$${path_file}                      \
                        $${path_ctrb}`/`basename $${file}`               \
            ARCH-$${arch}/$${dest};                                      \
          $(if $(VERBOSE),(echo -n ,;))                                  \
        fi;                                                              \
      fi;                                                                \
    done;                                                                \
  done

.PHONY : CONTRIB
CONTRIB:
	@echo -n "  ... Checking and fixing contrib file links "
	$(VERBOSE)$(REBUILD_LINKS)
	@echo "  ... done!"

else

CONTRIB:

endif

cleanall::
	$(VERBOSE)find . -type l | xargs rm -f$(if $(VERBOSE),,v)
	$(VERBOSE)$(RM) $(PKGDIR)/lib/uclibc/ARCH-*/include/.general.d

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
