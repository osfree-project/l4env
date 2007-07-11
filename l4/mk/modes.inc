# -*- Makefile -*-
# vim:et:ts=8:ft=make:
#
# Makefile-Include for defining modes

# Default memory backend when linking against dietlibc or uClibc. The -u
# switch is necessary since mmap_anon is referenced using a weak symbol.
MALLOC_BACKEND     ?= -u mmap_anon -lc_be_simple_mem

MMAP_BACKEND       ?= -lc_be_sigma0_mem

# Default is dietlibc and uClibc without reentrant functions. If, for example
# two independant threads of the same application call malloc at the same
# time, WANT_THREADSAFE _must_ be defined.
ifneq ("$(WANT_THREADSAFE)","")
DIETLIBC_IMPLEMENTATION := -ldiet_c_r
UCLIBC_IMPLEMENTATION   := -luc_c_r
else
DIETLIBC_IMPLEMENTATION := -ldiet_c
UCLIBC_IMPLEMENTATION   := -luc_c
endif

ROOTLIB  = -lroot
ROOTPLIB = -lroot.p

CRT0_DEFAULT = $(call findfile,crt0.o,$(L4LIBDIR))
CRTP_DEFAULT = $(call findfile,crt0.s.o,$(L4LIBDIR))
LDS_DEFAULT  = $(call findfile,main_stat.ld,$(L4LIBDIR))

ifeq ($(MODE),loader)
LDS_LOADER   = $(call findfile,main_dyn.ld,$(L4LIBDIR))
else
LDS_LOADER   = $(LDS_DEFAULT)
endif

LIBC_INCLUDE_diet = -nostdinc                                 \
                    -I$(OBJ_BASE)/include/dietlibc            \
                    -I$(OBJ_BASE)/include/uclibc++            \
                    -I$(DROPS_STDDIR)/include/dietlibc        \
                    -I$(DROPS_STDDIR)/include/uclibc++        \
                    -I$(GCCINCDIR)
LIBC_LIB_diet     = -ldiet_c
LIBC_INCLUDE_uc   = -nostdinc                                 \
                    -I$(OBJ_BASE)/include/$(ARCH)/uclibc      \
                    -I$(OBJ_BASE)/include/uclibc              \
                    -I$(OBJ_BASE)/include/uclibc++            \
                    -I$(DROPS_STDDIR)/include/$(ARCH)/uclibc  \
                    -I$(DROPS_STDDIR)/include/uclibc          \
                    -I$(DROPS_STDDIR)/include/uclibc++        \
                    -I$(GCCINCDIR)
LIBC_LIB_uc       = -luc_c

ifeq ($(USE_DIETLIBC),y)
LIBC_INCLUDE = $(LIBC_INCLUDE_diet)
LIBC_LIB     = $(LIBC_LIB_diet)
else
LIBC_INCLUDE = $(LIBC_INCLUDE_uc)
LIBC_LIB     = $(LIBC_LIB_uc)
endif

#############################################################################
#
# X86 (IA32)
#

#
# x86_tiny - minimal environment (only l4util, no log, no names)
#
BID_SUPPORTED_x86_tiny_diet     = y
LIBCINCDIR_x86_tiny_diet        = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_x86_tiny_diet        =
LIBCLIBS_x86_tiny_diet          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_minimal_io                      \
                                  -ldiet_be_l4_start_stop                   \
                                  $(MMAP_BACKEND)                           \
                                  -ldiet_c -ll4util -lsigma0                \
                                  -ldiet_c $(GCCLIB) -ldiet_c -ll4sys
L4LIBS_x86_tiny_diet            = -static -lmain -ll4util -ll4sys
CRT0_x86_tiny_diet              = $(CRT0_DEFAULT)
CRTN_x86_tiny_diet              =
LDSCRIPT_x86_tiny_diet          = $(LDS_DEFAULT)

BID_SUPPORTED_x86_tiny_uc       = y
LIBCINCDIR_x86_tiny_uc          = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_x86_tiny_uc          =
LIBCLIBS_x86_tiny_uc            = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support -ll4util -lsigma0 \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_x86_tiny_uc              = $(L4LIBS_x86_tiny_diet)
CRT0_x86_tiny_uc                = $(CRT0_DEFAULT)
CRTN_x86_tiny_uc                =
LDSCRIPT_x86_tiny_uc            = $(LDS_DEFAULT)

BID_SUPPORTED_x86_tiny          = y
ifeq ($(USE_DIETLIBC),y)
  LIBCINCDIR_x86_tiny           = $(LIBCINCDIR_x86_tiny_diet)
  LIBCLIBDIR_x86_tiny           = $(LIBCLIBDIR_x86_tiny_diet)
  LIBCLIBS_x86_tiny             = $(LIBCLIBS_x86_tiny_diet)
  L4LIBS_x86_tiny               = $(L4LIBS_x86_tiny_diet)
  CRT0_x86_tiny                 = $(CRT0_x86_tiny_diet)
  CRTN_x86_tiny                 = $(CRTN_x86_tiny_diet)
  LDSCRIPT_x86_tiny             = $(LDSCRIPT_x86_tiny_diet)
else
  LIBCINCDIR_x86_tiny           = $(LIBCINCDIR_x86_tiny_uc)
  LIBCLIBDIR_x86_tiny           = $(LIBCLIBDIR_x86_tiny_uc)
  LIBCLIBS_x86_tiny             = $(LIBCLIBS_x86_tiny_uc)
  L4LIBS_x86_tiny               = $(L4LIBS_x86_tiny_uc)
  CRT0_x86_tiny                 = $(CRT0_x86_tiny_uc)
  CRTN_x86_tiny                 = $(CRTN_x86_tiny_uc)
  LDSCRIPT_x86_tiny             = $(LDSCRIPT_x86_tiny_uc)
endif


#
# x86_sigma0 - minimal environment (util, names, log)
#
BID_SUPPORTED_x86_sigma0_diet   = y
LIBCINCDIR_x86_sigma0_diet      = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_x86_sigma0_diet      =
LIBCLIBS_x86_sigma0_diet        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_minimal_io                      \
                                  -ldiet_be_l4_start_stop                   \
                                  $(MMAP_BACKEND)                           \
                                  -ldiet_c                                  \
                                  -llogserver -lnames -lsigma0              \
                                  -ll4util_root -ll4util $(ROOTLIB)         \
                                  -ldiet_c $(GCCLIB) -ldiet_c -ll4sys
L4LIBS_x86_sigma0_diet          = -static -lmain -lnames                    \
                                  -llogserver -ll4util_root -ll4util -ll4sys
CRT0_x86_sigma0_diet            = $(CRT0_DEFAULT)
CRTN_x86_sigma0_diet            =
LDSCRIPT_x86_sigma0_diet        = $(LDS_DEFAULT)

BID_SUPPORTED_x86_sigma0_uc     = y
LIBCINCDIR_x86_sigma0_uc        = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_x86_sigma0_uc        =
LIBCLIBS_x86_sigma0_uc          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support                   \
                                  -llogserver -lnames -lsigma0              \
                                  -ll4util_root -ll4util $(ROOTLIB)         \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_x86_sigma0_uc            = $(L4LIBS_x86_sigma0_diet)
CRT0_x86_sigma0_uc              = $(CRT0_DEFAULT)
CRTN_x86_sigma0_uc              =
LDSCRIPT_x86_sigma0_uc          = $(LDS_DEFAULT)

BID_SUPPORTED_x86_sigma0        = y
ifeq ($(USE_DIETLIBC),y)
  LIBCINCDIR_x86_sigma0         = $(LIBCINCDIR_x86_sigma0_diet)
  LIBCLIBDIR_x86_sigma0         = $(LIBCLIBDIR_x86_sigma0_diet)
  LIBCLIBS_x86_sigma0           = $(LIBCLIBS_x86_sigma0_diet)
  L4LIBS_x86_sigma0             = $(L4LIBS_x86_sigma0_diet)
  CRT0_x86_sigma0               = $(CRT0_x86_sigma0_diet)
  CRTN_x86_sigma0               = $(CRTN_x86_sigma0_diet)
  LDSCRIPT_x86_sigma0           = $(LDSCRIPT_x86_sigma0_diet)
else
  LIBCINCDIR_x86_sigma0         = $(LIBCINCDIR_x86_sigma0_uc)
  LIBCLIBDIR_x86_sigma0         = $(LIBCLIBDIR_x86_sigma0_uc)
  LIBCLIBS_x86_sigma0           = $(LIBCLIBS_x86_sigma0_uc)
  L4LIBS_x86_sigma0             = $(L4LIBS_x86_sigma0_uc)
  CRT0_x86_sigma0               = $(CRT0_x86_sigma0_uc)
  CRTN_x86_sigma0               = $(CRTN_x86_sigma0_uc)
  LDSCRIPT_x86_sigma0           = $(LDSCRIPT_x86_sigma0_uc)
endif


#
# all_libc - compiles C libs
#
BID_SUPPORTED_all_libc          = y
LIBCINCDIR_all_libc             = $(LIBC_INCLUDE)
LIBCLIBDIR_all_libc             =
LIBCLIBS_all_libc               = -nostdlib $(GCCLDNOSTDLIB) $(LIBC_LIB)    \
                                  $(LIBCBACKEND_LIB) $(GCCLIB) $(LIBC_LIB)
L4LIBS_all_libc                 =
CRT0_all_libc                   = $(CRT0_DEFAULT)
CRTN_all_libc                   =
LDSCRIPT_all_libc               = $(LDS_DEFAULT)


#
# all_linux - compilation without anything, but unlike host mode still
#             (potentially) cross compiled
#
BID_SUPPORTED_all_linux         = y
LIBCINCDIR_all_linux            =
LIBCLIBDIR_all_linux            =
LIBCLIBS_all_linux              =
L4LIBS_all_linux                =
CRT0_all_linux                  =
CRTN_all_linux                  =
LDSCRIPT_all_linux              =


#
# all_host - compilation for the host system
#
BID_SUPPORTED_all_host          = y
LIBCINCDIR_all_host             =
LIBCLIBDIR_all_host             =
LIBCLIBS_all_host               =
L4LIBS_all_host                 =
CRT0_all_host                   =
CRTN_all_host                   =
LDSCRIPT_all_host               =

#
# x86_l4linux - L4Env functions for L4Linux programs
#
BID_SUPPORTED_x86_l4linux       = y
LIBCINCDIR_x86_l4linux          = -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4linux          = -L$(DROPS_STDDIR)/lib
LIBCLIBS_x86_l4linux            = -ldm_generic -ldm_mem -lnames             \
                                  $(ROOTLIB) -lloaderif -ll4env             \
                                  -ll4env_err -lslab                        \
                                  -llogserver_capsule -ll4util -ll4sys
CRT0_x86_l4linux                =
CRTN_x86_l4linux                =
LDSCRIPT_x86_l4linux            =


#
# x86_l4linux_kern - build L4Linux kernel modules
#
BID_SUPPORTED_x86_l4linux_kern  = y
LIBCINCDIR_x86_l4linux_kern     = -nostdinc -I$(L4LINUX22_DIR)/include      \
                                  -D__KERNEL__ -DLINUX_ON_L4 -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4linux_kern     =
LIBCLIBS_x86_l4linux_kern       =
CRT0_x86_l4linux_kern           =
CRTN_x86_l4linux_kern           =
LDSCRIPT_x86_l4linux_kern       =


#
# x86_l4env -- l4env, the default
#
BID_SUPPORTED_x86_l4env_diet    = y
LIBCINCDIR_x86_l4env_diet       = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_x86_l4env_diet       =
L4LIBS_x86_l4env_diet           = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule           \
                                  -lnames -ll4util_root -ll4util -lsigma0   \
                                  $(ROOTLIB) -ll4env -ll4env_err -ll4rm     \
                                  -ldm_generic -ldm_mem -lthread -lslab     \
                                  -ll4sys
LIBCLIBS_x86_l4env_diet         = -nostdlib $(GCCLDNOSTDLIB)                \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  -ldietlibc_support                        \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap                               \
                                  -lc_be_mmap_util                          \
                                  -lc_be_l4env_start_stop                   \
                                  -lc_be_minimal_log_io                     \
                                  -ldiet_be_simple_sleep                    \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -lgeneric_ts $(DIETLIBC_IMPLEMENTATION)   \
                                  $(GCCLIB) $(DIETLIBC_IMPLEMENTATION)      \
                                  -ll4sys
CRT0_x86_l4env_diet             = $(CRT0_DEFAULT)
CRTN_x86_l4env_diet             =
LDSCRIPT_x86_l4env_diet         = $(LDS_DEFAULT)

BID_SUPPORTED_x86_l4env_uc      = y
LIBCINCDIR_x86_l4env_uc         = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_x86_l4env_uc         =
L4LIBS_x86_l4env_uc             = $(L4LIBS_x86_l4env_diet)
LIBCLIBS_x86_l4env_uc           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  -luclibc_support                          \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap                               \
                                  -lc_be_mmap_util                          \
                                  -lc_be_l4env_start_stop                   \
                                  -lc_be_minimal_log_io                     \
                                  -luc_be_simple_sleep                      \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -lgeneric_ts $(UCLIBC_IMPLEMENTATION)     \
                                  $(GCCLIB) $(UCLIBC_IMPLEMENTATION)        \
                                  -ll4sys
CRT0_x86_l4env_uc               = $(CRT0_DEFAULT)
CRTN_x86_l4env_uc               =
LDSCRIPT_x86_l4env_uc           = $(LDS_DEFAULT)

BID_SUPPORTED_x86_l4env         = y
ifeq ($(USE_DIETLIBC),y)
  LIBCINCDIR_x86_l4env          = $(LIBCINCDIR_x86_l4env_diet)
  LIBCLIBDIR_x86_l4env          = $(LIBCLIBDIR_x86_l4env_diet)
  L4LIBS_x86_l4env              = $(L4LIBS_x86_l4env_diet)
  LIBCLIBS_x86_l4env            = $(LIBCLIBS_x86_l4env_diet)
  CRT0_x86_l4env                = $(CRT0_x86_l4env_diet)
  CRTN_x86_l4env                = $(CRTN_x86_l4env_diet)
  LDSCRIPT_x86_l4env            = $(LDSCRIPT_x86_l4env_diet)
else
  LIBCINCDIR_x86_l4env          = $(LIBCINCDIR_x86_l4env_uc)
  LIBCLIBDIR_x86_l4env          = $(LIBCLIBDIR_x86_l4env_uc)
  L4LIBS_x86_l4env              = $(L4LIBS_x86_l4env_uc)
  LIBCLIBS_x86_l4env            = $(LIBCLIBS_x86_l4env_uc)
  CRT0_x86_l4env                = $(CRT0_x86_l4env_uc)
  CRTN_x86_l4env                = $(CRTN_x86_l4env_uc)
  LDSCRIPT_x86_l4env            = $(LDSCRIPT_x86_l4env_uc)
endif


#
# x86_l4env_minimal - dietlibc/uClibc with just write(1, ...) as io backend
#
BID_SUPPORTED_x86_l4env_dietlibc_minimal = y
LIBCINCDIR_x86_l4env_dietlibc_minimal = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_x86_l4env_dietlibc_minimal =
LIBCLIBS_x86_l4env_dietlibc_minimal = -nostdlib $(GCCLDNOSTDLIB)            \
                                  $(MALLOC_BACKEND)                         \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  -lc_be_l4env_start_stop -lgeneric_ts      \
                                  -lc_be_minimal_log_io -lc_be_mmap_util    \
                                  $(MALLOC_BACKEND) -lc_be_mmap             \
                                  -lc_be_mmap_util                          \
                                  -ll4rm -ldm_mem -ldm_generic -lthread     \
                                  -lsemaphore -ll4env -ll4env_err -lslab    \
                                  -llogserver_capsule -ll4rm -lthread       \
                                  -ldm_generic -lnames                      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(DIETLIBC_IMPLEMENTATION) $(GCCLIB)      \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  -lc_be_l4env_start_stop -ll4sys
L4LIBS_x86_l4env_dietlibc_minimal = -static -ll4rm -ldm_mem                 \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4sys
CRT0_x86_l4env_dietlibc_minimal = $(CRT0_DEFAULT)
CRTN_x86_l4env_dietlibc_minimal =
LDSCRIPT_x86_l4env_dietlibc_minimal = $(LDS_DEFAULT)

BID_SUPPORTED_x86_l4env_uclibc_minimal = y
LIBCINCDIR_x86_l4env_uclibc_minimal = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_x86_l4env_uclibc_minimal =
LIBCLIBS_x86_l4env_uclibc_minimal = -nostdlib $(GCCLDNOSTDLIB)              \
                                  $(MALLOC_BACKEND)                         \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  -lc_be_l4env_start_stop -lgeneric_ts      \
                                  -lc_be_minimal_log_io -lc_be_mmap_util    \
                                  $(MALLOC_BACKEND) -lc_be_mmap             \
                                  -lc_be_mmap_util                          \
                                  -ll4rm -ldm_mem -ldm_generic -lthread     \
                                  -lsemaphore -ll4env -ll4env_err -lslab    \
                                  -llogserver_capsule -ll4rm -lthread       \
                                  -ldm_generic -lnames                      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(UCLIBC_IMPLEMENTATION) $(GCCLIB)        \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  -lc_be_l4env_start_stop -luclibc_support  \
                                  -ll4sys
L4LIBS_x86_l4env_uclibc_minimal = $(L4LIBS_x86_l4env_dietlibc_minimal)
CRT0_x86_l4env_uclibc_minimal   = $(CRT0_DEFAULT)
CRTN_x86_l4env_uclibc_minimal   =
LDSCRIPT_x86_l4env_uclibc_minimal = $(LDS_DEFAULT)

BID_SUPPORTED_x86_l4env_minimal = y
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_x86_l4env_minimal  = $(LIBCINCDIR_x86_l4env_uclibc_minimal)
  LIBCLIBDIR_x86_l4env_minimal  = $(LIBCLIBDIR_x86_l4env_uclibc_minimal)
  L4LIBS_x86_l4env_minimal      = $(L4LIBS_x86_l4env_uclibc_minimal)
  LIBCLIBS_x86_l4env_minimal    = $(LIBCLIBS_x86_l4env_uclibc_minimal)
  CRT0_x86_l4env_minimal        = $(CRT0_x86_l4env_uclibc_minimal)
  CRTN_x86_l4env_minimal        = $(CRTN_x86_l4env_uclibc_minimal)
  LDSCRIPT_x86_l4env_minimal    = $(LDSCRIPT_x86_l4env_uclibc_minimal)
else
  LIBCINCDIR_x86_l4env_minimal  = $(LIBCINCDIR_x86_l4env_dietlibc_minimal)
  LIBCLIBDIR_x86_l4env_minimal  = $(LIBCLIBDIR_x86_l4env_dietlibc_minimal)
  L4LIBS_x86_l4env_minimal      = $(L4LIBS_x86_l4env_dietlibc_minimal)
  LIBCLIBS_x86_l4env_minimal    = $(LIBCLIBS_x86_l4env_dietlibc_minimal)
  CRT0_x86_l4env_minimal        = $(CRT0_x86_l4env_dietlibc_minimal)
  CRTN_x86_l4env_minimal        = $(CRTN_x86_l4env_dietlibc_minimal)
  LDSCRIPT_x86_l4env_minimal    = $(LDSCRIPT_x86_l4env_dietlibc_minimal)
endif


#
# x86_l4env_base - dietlibc/uclibc with backends:
#     basic_io, io, basic_mmap, mmap_util, syslog, simple_sleep, time,
#     file_table
#
LIBCINCDIR_x86_l4env_base_diet  = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_x86_l4env_base_diet  =
LIBCLIBS_x86_l4env_base_diet    = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -u printf -lc_be_io.o $(MALLOC_BACKEND)   \
                                  -lc_be_time -lrtc -ll4rm -ldm_mem         \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  $(GCCLIB) $(DIETLIBC_IMPLEMENTATION)      \
                                  $(MALLOC_BACKEND) -ll4rm -ldm_mem         \
                                  -ldm_generic -lc_be_time -lc_be_mmap      \
                                  -lc_be_mmap_util -lc_be_l4env_start_stop  \
                                  -lgeneric_ts                              \
                                  -lc_be_syslog -lc_be_file-table           \
                                  -ldiet_be_simple_sleep -ll4vfs_common_io  \
                                  -ll4vfs_basic_io -ll4vfs_connection       \
                                  -ll4vfs_basic_name_server                 \
                                  -ll4vfs_name_server                       \
                                  -ll4vfs_name_space_provider               \
                                  -ll4vfs_extendable
L4LIBS_x86_l4env_base_diet      = -static -ll4rm -ldm_mem -ldm_generic      \
                                  -lthread -lsemaphore -ll4env -ll4env_err  \
                                  -lslab -llogserver_capsule -ll4rm         \
                                  -lthread -ldm_generic -lnames             \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4sys
CRT0_x86_l4env_base_diet        = $(CRT0_DEFAULT)
CRTN_x86_l4env_base_diet        =
LDSCRIPT_x86_l4env_base_diet    = $(LDS_DEFAULT)

BID_SUPPORTED_x86_l4env_base_uc = y
LIBCINCDIR_x86_l4env_base_uc    = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_x86_l4env_base_uc    =
LIBCLIBS_x86_l4env_base_uc      = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -u printf -lc_be_io.o $(MALLOC_BACKEND)   \
                                  -lc_be_time -lrtc -ll4rm -ldm_mem         \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  $(GCCLIB) $(UCLIBC_IMPLEMENTATION)        \
                                  $(MALLOC_BACKEND) -ll4rm -ldm_mem         \
                                  -ldm_generic -lc_be_time -lc_be_mmap      \
                                  -lc_be_mmap_util -lc_be_l4env_start_stop  \
                                  -lgeneric_ts                              \
                                  -lc_be_syslog -lc_be_file-table           \
                                  -luc_be_simple_sleep -ll4vfs_common_io    \
                                  -ll4vfs_basic_io -ll4vfs_connection       \
                                  -ll4vfs_basic_name_server                 \
                                  -ll4vfs_name_server                       \
                                  -ll4vfs_name_space_provider               \
                                  -ll4vfs_extendable -ll4sys
L4LIBS_x86_l4env_base_uc        = $(L4LIBS_x86_l4env_base_diet)
CRT0_x86_l4env_base_uc          = $(CRT0_DEFAULT)
CRTN_x86_l4env_base_uc          =
LDSCRIPT_x86_l4env_base_uc      = $(LDS_DEFAULT)

BID_SUPPORTED_x86_l4env_base    = y
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_x86_l4env_base     = $(LIBCINCDIR_x86_l4env_base_uc)
  LIBCLIBDIR_x86_l4env_base     = $(LIBCLIBDIR_x86_l4env_base_uc)
  L4LIBS_x86_l4env_base         = $(L4LIBS_x86_l4env_base_uc)
  LIBCLIBS_x86_l4env_base       = $(LIBCLIBS_x86_l4env_base_uc)
  CRT0_x86_l4env_base           = $(CRT0_x86_l4env_base_uc)
  CRTN_x86_l4env_base           = $(CRTN_x86_l4env_base_uc)
  LDSCRIPT_x86_l4env_base       = $(LDSCRIPT_x86_l4env_base_uc)
else
  LIBCINCDIR_x86_l4env_base     = $(LIBCINCDIR_x86_l4env_base_diet)
  LIBCLIBDIR_x86_l4env_base     = $(LIBCLIBDIR_x86_l4env_base_diet)
  L4LIBS_x86_l4env_base         = $(L4LIBS_x86_l4env_base_diet)
  LIBCLIBS_x86_l4env_base       = $(LIBCLIBS_x86_l4env_base_diet)
  CRT0_x86_l4env_base           = $(CRT0_x86_l4env_base_diet)
  CRTN_x86_l4env_base           = $(CRTN_x86_l4env_base_diet)
  LDSCRIPT_x86_l4env_base       = $(LDSCRIPT_x86_l4env_base_diet)
endif


#
# x86_loader - applications to be loaded by the L4Env loader.
#              Will be merged with l4env_base and become the default mode.
#
BID_SUPPORTED_x86_loader      = y
LIBCINCDIR_x86_loader         = $(LIBC_INCLUDE)
LIBCLIBDIR_x86_loader         =
ifeq ($(USE_DIETLIBC),y)
  LIBCLIBS_x86_loader           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c $(GCCLIB) -ldiet_c
else
  LIBCLIBS_x86_loader           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c $(GCCLIB) -luc_c
endif
L4LIBS_x86_loader             = -lloader.s -ll4sys
CRT0_x86_loader               = $(CRT0_DEFAULT)
CRTN_x86_loader               =
LDSCRIPT_x86_loader           = $(LDS_LOADER)


#############################################################################
#
# ARM
#

#
# arm_tiny
#
BID_SUPPORTED_arm_tiny_diet     = y
LIBCINCDIR_arm_tiny_diet        = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_arm_tiny_diet        =
LIBCLIBS_arm_tiny_diet          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain -ldiet_c -ldietlibc_support        \
                                  -ldiet_be_l4_start_stop                   \
                                  -ldiet_be_minimal_io                      \
                                  $(MMAP_BACKEND)                           \
                                  -lsigma0 $(GCCLIB) -ldiet_c
L4LIBS_arm_tiny_diet            = -static -lmain -ll4util -ll4sys
CRT0_arm_tiny_diet              = $(CRT0_DEFAULT)
CRTN_arm_tiny_diet              =
LDSCRIPT_arm_tiny_diet          = $(LDS_DEFAULT)

BID_SUPPORTED_arm_tiny_uc       = y
LIBCINCDIR_arm_tiny_uc          = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_arm_tiny_uc          =
LIBCLIBS_arm_tiny_uc            = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain -luc_c -luclibc_support            \
                                  -luc_be_l4_start_stop                     \
                                  -luc_be_minimal_io                        \
                                  $(MMAP_BACKEND)                           \
                                  -lsigma0 $(GCCLIB) -luc_c
L4LIBS_arm_tiny_uc              = $(L4LIBS_arm_tiny_diet)
CRT0_arm_tiny_uc                = $(CRT0_DEFAULT)
CRTN_arm_tiny_uc                =
LDSCRIPT_arm_tiny_uc            = $(LDS_DEFAULT)

BID_SUPPORTED_arm_tiny          = y
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_arm_tiny           = $(LIBCINCDIR_arm_tiny_uc)
  LIBCLIBDIR_arm_tiny           = $(LIBCLIBDIR_arm_tiny_uc)
  LIBCLIBS_arm_tiny             = $(LIBCLIBS_arm_tiny_uc)
  L4LIBS_arm_tiny               = $(L4LIBS_arm_tiny_uc)
  CRT0_arm_tiny                 = $(CRT0_arm_tiny_uc)
  CRTN_arm_tiny                 = $(CRTN_arm_tiny_uc)
  LDSCRIPT_arm_tiny             = $(LDSCRIPT_arm_tiny_uc)
else
  LIBCINCDIR_arm_tiny           = $(LIBCINCDIR_arm_tiny_diet)
  LIBCLIBDIR_arm_tiny           = $(LIBCLIBDIR_arm_tiny_diet)
  LIBCLIBS_arm_tiny             = $(LIBCLIBS_arm_tiny_diet)
  L4LIBS_arm_tiny               = $(L4LIBS_arm_tiny_diet)
  CRT0_arm_tiny                 = $(CRT0_arm_tiny_diet)
  CRTN_arm_tiny                 = $(CRTN_arm_tiny_diet)
  LDSCRIPT_arm_tiny             = $(LDSCRIPT_arm_tiny_diet)
endif


BID_SUPPORTED_arm_sigma0_diet   = y
LIBCINCDIR_arm_sigma0_diet      = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_arm_sigma0_diet      =
LIBCLIBS_arm_sigma0_diet        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain                                    \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_l4_start_stop                   \
                                  -ldiet_be_minimal_io                      \
                                  $(MMAP_BACKEND)                           \
                                  -lsigma0 $(GCCLIB) -ldiet_c
L4LIBS_arm_sigma0_diet          = -static -lmain -llogserver -lnames        \
                                  -ll4util_root -ll4util $(ROOTLIB) -ll4sys
CRT0_arm_sigma0_diet            = $(CRT0_DEFAULT)
CRTN_arm_sigma0_diet            =
LDSCRIPT_arm_sigma0_diet        = $(LDS_DEFAULT)

BID_SUPPORTED_arm_sigma0_uc     = y
LIBCINCDIR_arm_sigma0_uc        = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_arm_sigma0_uc        =
LIBCLIBS_arm_sigma0_uc          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain                                    \
                                  -luc_c                                    \
                                  -luclibc_support                          \
                                  -luc_be_l4_start_stop                     \
                                  -luc_be_minimal_io                        \
                                  $(MMAP_BACKEND)                           \
                                  -lsigma0 $(GCCLIB) -luc_c
L4LIBS_arm_sigma0_uc            = $(L4LIBS_arm_sigma0_diet)
CRT0_arm_sigma0_uc              = $(CRT0_DEFAULT)
CRTN_arm_sigma0_uc              =
LDSCRIPT_arm_sigma0_uc          = $(LDS_DEFAULT)

BID_SUPPORTED_arm_sigma0        = y
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_arm_sigma0         = $(LIBCINCDIR_arm_sigma0_uc)
  LIBCLIBDIR_arm_sigma0         = $(LIBCLIBDIR_arm_sigma0_uc)
  LIBCLIBS_arm_sigma0           = $(LIBCLIBS_arm_sigma0_uc)
  L4LIBS_arm_sigma0             = $(L4LIBS_arm_sigma0_uc)
  CRT0_arm_sigma0               = $(CRT0_arm_sigma0_uc)
  CRTN_arm_sigma0               = $(CRTN_arm_sigma0_uc)
  LDSCRIPT_arm_sigma0           = $(LDSCRIPT_arm_sigma0_uc)
else
  LIBCINCDIR_arm_sigma0         = $(LIBCINCDIR_arm_sigma0_diet)
  LIBCLIBDIR_arm_sigma0         = $(LIBCLIBDIR_arm_sigma0_diet)
  LIBCLIBS_arm_sigma0           = $(LIBCLIBS_arm_sigma0_diet)
  L4LIBS_arm_sigma0             = $(L4LIBS_arm_sigma0_diet)
  CRT0_arm_sigma0               = $(CRT0_arm_sigma0_diet)
  CRTN_arm_sigma0               = $(CRTN_arm_sigma0_diet)
  LDSCRIPT_arm_sigma0           = $(LDSCRIPT_arm_sigma0_diet)
endif


#
# arm_l4env_minimal -- just a copy of sigma0 mode, so that not so many
# makefile changes are necessary
#
BID_SUPPORTED_arm_l4env_dietlibc_minimal    = $(BID_SUPPORTED_arm_sigma0_diet)
LIBCINCDIR_arm_l4env_dietlibc_minimal       = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_arm_l4env_dietlibc_minimal       = $(LIBCLIBDIR_arm_sigma0_diet)
LIBCLIBS_arm_l4env_dietlibc_minimal         = $(LIBCLIBS_arm_sigma0_diet)
L4LIBS_arm_l4env_dietlibc_minimal           = $(L4LIBS_arm_sigma0_diet)
CRT0_arm_l4env_dietlibc_minimal             = $(CRT0_arm_sigma0_diet)
CRTN_arm_l4env_dietlibc_minimal             = $(CRTN_arm_sigma0_diet)
LDSCRIPT_arm_l4env_dietlibc_minimal         = $(LDSCRIPT_arm_sigma0_diet)

BID_SUPPORTED_arm_l4env_uclibc_minimal      = $(BID_SUPPORTED_arm_sigma0_uc)
LIBCINCDIR_arm_l4env_uclibc_minimal         = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_arm_l4env_uclibc_minimal         = $(LIBCLIBDIR_arm_sigma0_uc)
LIBCLIBS_arm_l4env_uclibc_minimal           = $(LIBCLIBS_arm_sigma0_uc)
L4LIBS_arm_l4env_uclibc_minimal             = $(L4LIBS_arm_sigma0_uc)
CRT0_arm_l4env_uclibc_minimal               = $(CRT0_arm_sigma0_uc)
CRTN_arm_l4env_uclibc_minimal               = $(CRTN_arm_sigma0_uc)
LDSCRIPT_arm_l4env_uclibc_minimal           = $(LDSCRIPT_arm_sigma0_uc)

BID_SUPPORTED_arm_l4env_minimal = $(BID_SUPPORTED_arm_sigma0)
LIBCINCDIR_arm_l4env_minimal    = $(LIBCINCDIR_arm_sigma0)
LIBCLIBDIR_arm_l4env_minimal    = $(LIBCLIBDIR_arm_sigma0)
LIBCLIBS_arm_l4env_minimal      = $(LIBCLIBS_arm_sigma0)
L4LIBS_arm_l4env_minimal        = $(L4LIBS_arm_sigma0)
CRT0_arm_l4env_minimal          = $(CRT0_arm_sigma0)
CRTN_arm_l4env_minimal          = $(CRTN_arm_sigma0)
LDSCRIPT_arm_l4env_minimal      = $(LDSCRIPT_arm_sigma0)

#
# arm_l4env -- default mode
#
BID_SUPPORTED_arm_l4env         = y
LIBCINCDIR_arm_l4env_diet       = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_arm_l4env_diet       =
L4LIBS_arm_l4env_diet           = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule -lnames   \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4env -ll4env_err            \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lslab -ll4sys
LIBCLIBS_arm_l4env_diet         = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c -ldietlibc_support               \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap -lc_be_mmap_util              \
                                  -ldiet_be_l4_start_stop                   \
                                  -lc_be_minimal_log_io                     \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -ldiet_c $(GCCLIB) -ldiet_c
CRT0_arm_l4env_diet             = $(CRT0_DEFAULT)
CRTN_arm_l4env_diet             =
LDSCRIPT_arm_l4env_diet         = $(LDS_DEFAULT)

BID_SUPPORTED_arm_l4env_uc      = y
LIBCINCDIR_arm_l4env_uc         = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_arm_l4env_uc         =
L4LIBS_arm_l4env_uc             = $(L4LIBS_arm_l4env_diet)
LIBCLIBS_arm_l4env_uc           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c -luclibc_support                   \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap -lc_be_mmap_util              \
                                  -luc_be_l4_start_stop                     \
                                  -lc_be_minimal_log_io                     \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -luc_c $(GCCLIB) -luc_c
CRT0_arm_l4env_uc               = $(CRT0_DEFAULT)
CRTN_arm_l4env_uc               =
LDSCRIPT_arm_l4env_uc           = $(LDS_DEFAULT)

ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_arm_l4env          = $(LIBCINCDIR_arm_l4env_uc)
  LIBCLIBDIR_arm_l4env          = $(LIBCLIBDIR_arm_l4env_uc)
  L4LIBS_arm_l4env              = $(L4LIBS_arm_l4env_uc)
  LIBCLIBS_arm_l4env            = $(LIBCLIBS_arm_l4env_uc)
  CRT0_arm_l4env                = $(CRT0_arm_l4env_uc)
  CRTN_arm_l4env                = $(CRTN_arm_l4env_uc)
  LDSCRIPT_arm_l4env            = $(LDSCRIPT_arm_l4env_uc)
else
  LIBCINCDIR_arm_l4env          = $(LIBCINCDIR_arm_l4env_diet)
  LIBCLIBDIR_arm_l4env          = $(LIBCLIBDIR_arm_l4env_diet)
  L4LIBS_arm_l4env              = $(L4LIBS_arm_l4env_diet)
  LIBCLIBS_arm_l4env            = $(LIBCLIBS_arm_l4env_diet)
  CRT0_arm_l4env                = $(CRT0_arm_l4env_diet)
  CRTN_arm_l4env                = $(CRTN_arm_l4env_diet)
  LDSCRIPT_arm_l4env            = $(LDSCRIPT_arm_l4env_diet)
endif


#
# arm_loader - applications to be loaded by the L4Env loader.
#
LIBCINCDIR_arm_loader_diet      = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_arm_loader_diet      =
LIBCLIBS_arm_loader_diet        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c $(GCCLIB) -ldiet_c
L4LIBS_arm_loader_diet          = -lloader.s -ll4sys
CRT0_arm_loader_diet            = $(CRT0_DEFAULT)
CRTN_arm_loader_diet            =
LDSCRIPT_arm_loader_diet        = $(LDS_LOADER)

LIBCINCDIR_arm_loader_uc        = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_arm_loader_uc        =
LIBCLIBS_arm_loader_uc          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c $(GCCLIB) -luc_c
L4LIBS_arm_loader_uc            = -lloader.s -ll4sys
CRT0_arm_loader_uc              = $(CRT0_DEFAULT)
CRTN_arm_loader_uc              =
LDSCRIPT_arm_loader_uc          = $(LDS_LOADER)

BID_SUPPORTED_arm_loader        = y
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_arm_loader         = $(LIBCINCDIR_arm_loader_uc)
  LIBCLIBDIR_arm_loader         = $(LIBCLIBDIR_arm_loader_uc)
  L4LIBS_arm_loader             = $(L4LIBS_arm_loader_uc)
  LIBCLIBS_arm_loader           = $(LIBCLIBS_arm_loader_uc)
  CRT0_arm_loader               = $(CRT0_arm_loader_uc)
  CRTN_arm_loader               = $(CRTN_arm_loader_uc)
  LDSCRIPT_arm_loader           = $(LDSCRIPT_arm_loader_uc)
else
  LIBCINCDIR_arm_loader         = $(LIBCINCDIR_arm_loader_diet)
  LIBCLIBDIR_arm_loader         = $(LIBCLIBDIR_arm_loader_diet)
  L4LIBS_arm_loader             = $(L4LIBS_arm_loader_diet)
  LIBCLIBS_arm_loader           = $(LIBCLIBS_arm_loader_diet)
  CRT0_arm_loader               = $(CRT0_arm_loader_diet)
  CRTN_arm_loader               = $(CRTN_arm_loader_diet)
  LDSCRIPT_arm_loader           = $(LDSCRIPT_arm_loader_diet)
endif


#
# arm_l4linux - L4Env functions for L4Linux programs
#
BID_SUPPORTED_arm_l4linux       = y
LIBCINCDIR_arm_l4linux          = -I$(GCCINCDIR)
LIBCLIBDIR_arm_l4linux          = -L$(DROPS_STDDIR)/lib
LIBCLIBS_arm_l4linux            = -ldm_generic -ldm_mem -lnames             \
                                  $(ROOTLIB) -lloaderif -ll4env             \
                                  -ll4env_err -lslab                        \
                                  -llogserver_capsule -ll4util -ll4sys
CRT0_arm_l4linux                =
CRTN_arm_l4linux                =
LDSCRIPT_arm_l4linux            =


#############################################################################
#
# AMD64 
#

#
# amd64_tiny (without names and logserver)
#
BID_SUPPORTED_amd64_tiny_diet   = y
LIBCINCDIR_amd64_tiny_diet      = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_amd64_tiny_diet      =
LIBCLIBS_amd64_tiny_diet        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain                                    \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_l4_start_stop                   \
                                  -ldiet_be_minimal_io                      \
                                  $(MMAP_BACKEND) -lsigma0                  \
                                  -ldiet_c $(GCCLIB) -ldiet_c -ll4sys
L4LIBS_amd64_tiny_diet          = -static -lmain -ll4util -ll4sys
CRT0_amd64_tiny_diet            = $(CRT0_DEFAULT)
CRTN_amd64_tiny_diet            =
LDSCRIPT_amd64_tiny_diet        = $(LDS_DEFAULT)

BID_SUPPORTED_amd64_tiny_uc     = y
LIBCINCDIR_amd64_tiny_uc        = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_amd64_tiny_uc        =
LIBCLIBS_amd64_tiny_uc          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support -lsigma0          \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_amd64_tiny_uc            = $(L4LIBS_amd64_tiny_diet)
CRT0_amd64_tiny_uc              = $(CRT0_DEFAULT)
CRTN_amd64_tiny_uc              =
LDSCRIPT_amd64_tiny_uc          = $(LDS_DEFAULT)

BID_SUPPORTED_amd64_tiny        = y
ifeq ($(USE_DIETLIBC),y)
  LIBCINCDIR_amd64_tiny         = $(LIBCINCDIR_amd64_tiny_diet)
  LIBCLIBDIR_amd64_tiny         = $(LIBCLIBDIR_amd64_tiny_diet)
  LIBCLIBS_amd64_tiny           = $(LIBCLIBS_amd64_tiny_diet)
  L4LIBS_amd64_tiny             = $(L4LIBS_amd64_tiny_diet)
  CRT0_amd64_tiny               = $(CRT0_amd64_tiny_diet)
  CRTN_amd64_tiny               = $(CRTN_amd64_tiny_diet)
  LDSCRIPT_amd64_tiny           = $(LDSCRIPT_amd64_tiny_diet)
else
  LIBCINCDIR_amd64_tiny         = $(LIBCINCDIR_amd64_tiny_uc)
  LIBCLIBDIR_amd64_tiny         = $(LIBCLIBDIR_amd64_tiny_uc)
  LIBCLIBS_amd64_tiny           = $(LIBCLIBS_amd64_tiny_uc)
  L4LIBS_amd64_tiny             = $(L4LIBS_amd64_tiny_uc)
  CRT0_amd64_tiny               = $(CRT0_amd64_tiny_uc)
  CRTN_amd64_tiny               = $(CRTN_amd64_tiny_uc)
  LDSCRIPT_amd64_tiny           = $(LDSCRIPT_amd64_tiny_uc)
endif

#
# amd64_sigma0 - sigma0 with dietlibc
#
BID_SUPPORTED_amd64_sigma0_diet = y
LIBCINCDIR_amd64_sigma0_diet    = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_amd64_sigma0_diet    =
LIBCLIBS_amd64_sigma0_diet      = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_minimal_io                      \
                                  -ldiet_be_l4_start_stop                   \
                                  $(MMAP_BACKEND)                           \
                                  -ldiet_c                                  \
                                  -llogserver -lnames -lsigma0              \
                                  -ll4util_root -ll4util $(ROOTLIB)         \
                                  -ldiet_c $(GCCLIB) -ldiet_c -ll4sys
L4LIBS_amd64_sigma0_diet        = -static -lmain -lnames                    \
                                  -llogserver -ll4util_root -ll4util -ll4sys
CRT0_amd64_sigma0_diet          = $(CRT0_DEFAULT)
CRTN_amd64_sigma0_diet          =
LDSCRIPT_amd64_sigma0_diet      = $(LDS_DEFAULT)

BID_SUPPORTED_amd64_sigma0_uc   = y
LIBCINCDIR_amd64_sigma0_uc      = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_amd64_sigma0_uc      =
LIBCLIBS_amd64_sigma0_uc        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support                   \
                                  -llogserver -lnames -lsigma0              \
                                  -ll4util_root -ll4util $(ROOTLIB)         \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_amd64_sigma0_uc          = $(L4LIBS_amd64_sigma0_diet)
CRT0_amd64_sigma0_uc            = $(CRT0_DEFAULT)
CRTN_amd64_sigma0_uc            =
LDSCRIPT_amd64_sigma0_uc        = $(LDS_DEFAULT)

BID_SUPPORTED_amd64_sigma0 = y
ifeq ($(USE_DIETLIBC),y)
  LIBCINCDIR_amd64_sigma0       = $(LIBCINCDIR_amd64_sigma0_diet)
  LIBCLIBDIR_amd64_sigma0       = $(LIBCLIBDIR_amd64_sigma0_diet)
  LIBCLIBS_amd64_sigma0         = $(LIBCLIBS_amd64_sigma0_diet)
  L4LIBS_amd64_sigma0           = $(L4LIBS_amd64_sigma0_diet)
  CRT0_amd64_sigma0             = $(CRT0_amd64_sigma0_diet)
  CRTN_amd64_sigma0             = $(CRTN_amd64_sigma0_diet)
  LDSCRIPT_amd64_sigma0         = $(LDSCRIPT_amd64_sigma0_diet)
else
  LIBCINCDIR_amd64_sigma0       = $(LIBCINCDIR_amd64_sigma0_uc)
  LIBCLIBDIR_amd64_sigma0       = $(LIBCLIBDIR_amd64_sigma0_uc)
  LIBCLIBS_amd64_sigma0         = $(LIBCLIBS_amd64_sigma0_uc)
  L4LIBS_amd64_sigma0           = $(L4LIBS_amd64_sigma0_uc)
  CRT0_amd64_sigma0             = $(CRT0_amd64_sigma0_uc)
  CRTN_amd64_sigma0             = $(CRTN_amd64_sigma0_uc)
  LDSCRIPT_amd64_sigma0         = $(LDSCRIPT_amd64_sigma0_uc)
endif


#
# amd64_dietlibc minimal mode
#
BID_SUPPORTED_amd64_l4env_dietlibc_minimal  = $(BID_SUPPORTED_amd64_sigma0)
LIBCINCDIR_amd64_l4env_dietlibc_minimal     = $(LIBCINCDIR_amd64_sigma0)
LIBCLIBDIR_amd64_l4env_dietlibc_minimal     = $(LIBCLIBDIR_amd64_sigma0)
LIBCLIBS_amd64_l4env_dietlibc_minimal       = $(LIBCLIBS_amd64_sigma0)
L4LIBS_amd64_l4env_dietlibc_minimal         = $(L4LIBS_amd64_sigma0)
CRT0_amd64_l4env_dietlibc_minimal           = $(CRT0_amd64_sigma0)
CRTN_amd64_l4env_dietlibc_minimal           = $(CRTN_amd64_sigma0)
LDSCRIPT_amd64_l4env_dietlibc_minimal       = $(LDSCRIPT_amd64_sigma0)

#
# amd64_l4env - dietlibc and l4env, the default
#
BID_SUPPORTED_amd64_l4env_diet  = y
LIBCINCDIR_amd64_l4env_diet     = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_amd64_l4env_diet     =
L4LIBS_amd64_l4env_diet         = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule           \
                                  -lnames -ll4util_root -ll4util $(ROOTLIB) \
                                  -ll4env -ll4env_err -ll4rm -ldm_generic   \
                                  -ldm_mem -lthread -lslab -ll4sys
LIBCLIBS_amd64_l4env_diet       = -nostdlib $(GCCLDNOSTDLIB)                \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  -ldietlibc_support                        \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap                               \
                                  -lc_be_mmap_util                          \
                                  -lc_be_l4env_start_stop                   \
                                  -lc_be_minimal_log_io                     \
                                  -ldiet_be_simple_sleep                    \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -lgeneric_ts -lsigma0                     \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  $(GCCLIB) $(DIETLIBC_IMPLEMENTATION)      \
                                  -ll4sys
CRT0_amd64_l4env_diet           = $(CRT0_DEFAULT)
CRTN_amd64_l4env_diet           =
LDSCRIPT_amd64_l4env_diet       = $(LDS_DEFAULT)

LIBCINCDIR_amd64_l4env_uc         = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_amd64_l4env_uc         =
L4LIBS_amd64_l4env_uc             = $(L4LIBS_amd64_l4env_diet)
LIBCLIBS_amd64_l4env_uc           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  -luclibc_support                          \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap                               \
                                  -lc_be_mmap_util                          \
                                  -lc_be_l4env_start_stop                   \
                                  -lc_be_minimal_log_io                     \
                                  -luc_be_simple_sleep                      \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -lgeneric_ts -lsigma0                     \
								  $(UCLIBC_IMPLEMENTATION)     \
                                  $(GCCLIB) $(UCLIBC_IMPLEMENTATION)        \
                                  -ll4sys
CRT0_amd64_l4env_uc               = $(CRT0_DEFAULT)
CRTN_amd64_l4env_uc               =
LDSCRIPT_amd64_l4env_uc           = $(LDS_DEFAULT)
BID_SUPPORTED_amd64_l4env       = y
ifeq ($(USE_DIETLIBC),y)
  LIBCINCDIR_amd64_l4env        = $(LIBCINCDIR_amd64_l4env_diet)
  LIBCLIBDIR_amd64_l4env        = $(LIBCLIBDIR_amd64_l4env_diet)
  L4LIBS_amd64_l4env            = $(L4LIBS_amd64_l4env_diet)
  LIBCLIBS_amd64_l4env          = $(LIBCLIBS_amd64_l4env_diet)
  CRT0_amd64_l4env              = $(CRT0_amd64_l4env_diet)
  CRTN_amd64_l4env              = $(CRTN_amd64_l4env_diet)
  LDSCRIPT_amd64_l4env          = $(LDSCRIPT_amd64_l4env_diet)
endif
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_amd64_l4env        = $(LIBCINCDIR_amd64_l4env_uc)
  LIBCLIBDIR_amd64_l4env        = $(LIBCLIBDIR_amd64_l4env_uc)
  L4LIBS_amd64_l4env            = $(L4LIBS_amd64_l4env_uc)
  LIBCLIBS_amd64_l4env          = $(LIBCLIBS_amd64_l4env_uc)
  CRT0_amd64_l4env              = $(CRT0_amd64_l4env_uc)
  CRTN_amd64_l4env              = $(CRTN_amd64_l4env_uc)
  LDSCRIPT_amd64_l4env          = $(LDSCRIPT_amd64_l4env_uc)
endif

#
# amd64_l4env_minimal - dietlibc with just write(1, ...) as io backend
#
BID_SUPPORTED_amd64_l4env_dietlibc_minimal = y
LIBCINCDIR_amd64_l4env_dietlibc_minimal = $(LIBC_INCLUDE_diet)
LIBCLIBDIR_amd64_l4env_dietlibc_minimal =
LIBCLIBS_amd64_l4env_dietlibc_minimal = -nostdlib $(GCCLDNOSTDLIB)          \
                                  $(MALLOC_BACKEND)                         \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  -lc_be_l4env_start_stop -lgeneric_ts      \
                                  -lc_be_minimal_log_io -lc_be_mmap_util    \
                                  $(MALLOC_BACKEND) -lc_be_mmap             \
                                  -lc_be_mmap_util                          \
                                  -ll4rm -ldm_mem -ldm_generic -lthread     \
                                  -lsemaphore -ll4env -ll4env_err -lslab    \
                                  -llogserver_capsule -ll4rm -lthread       \
                                  -ldm_generic -lnames -ll4util             \
                                  $(DIETLIBC_IMPLEMENTATION) $(GCCLIB)      \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  -lc_be_l4env_start_stop -ll4sys
L4LIBS_amd64_l4env_dietlibc_minimal = -static -ll4rm -ldm_mem               \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util $(ROOTLIB) -ll4sys
CRT0_amd64_l4env_dietlibc_minimal = $(CRT0_DEFAULT)
CRTN_amd64_l4env_dietlibc_minimal =
LDSCRIPT_amd64_l4env_dietlibc_minimal = $(LDS_DEFAULT)

BID_SUPPORTED_amd64_l4env_uclibc_minimal = y
LIBCINCDIR_amd64_l4env_uclibc_minimal = $(LIBC_INCLUDE_uc)
LIBCLIBDIR_amd64_l4env_uclibc_minimal =
LIBCLIBS_amd64_l4env_uclibc_minimal = -nostdlib $(GCCLDNOSTDLIB)            \
                                  $(MALLOC_BACKEND)                         \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  -lc_be_l4env_start_stop -lgeneric_ts      \
                                  -lc_be_minimal_log_io -lc_be_mmap_util    \
                                  $(MALLOC_BACKEND) -lc_be_mmap             \
                                  -lc_be_mmap_util                          \
                                  -ll4rm -ldm_mem -ldm_generic -lthread     \
                                  -lsemaphore -ll4env -ll4env_err -lslab    \
                                  -llogserver_capsule -ll4rm -lthread       \
                                  -ldm_generic -lnames                      \
                                  -ll4util_root -ll4util                    \
                                  $(UCLIBC_IMPLEMENTATION) $(GCCLIB)        \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  -lc_be_l4env_start_stop -ll4sys
L4LIBS_amd64_l4env_uclibc_minimal = $(L4LIBS_amd64_l4env_dietlibc_minimal)
CRT0_amd64_l4env_uclibc_minimal   = $(CRT0_DEFAULT)
CRTN_amd64_l4env_uclibc_minimal   =
LDSCRIPT_amd64_l4env_uclibc_minimal = $(LDS_DEFAULT)

BID_SUPPORTED_amd64_l4env_minimal = y
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_amd64_l4env_minimal  = $(LIBCINCDIR_amd64_l4env_uclibc_minimal)
  LIBCLIBDIR_amd64_l4env_minimal  = $(LIBCLIBDIR_amd64_l4env_uclibc_minimal)
  L4LIBS_amd64_l4env_minimal      = $(L4LIBS_amd64_l4env_uclibc_minimal)
  LIBCLIBS_amd64_l4env_minimal    = $(LIBCLIBS_amd64_l4env_uclibc_minimal)
  CRT0_amd64_l4env_minimal        = $(CRT0_amd64_l4env_uclibc_minimal)
  CRTN_amd64_l4env_minimal        = $(CRTN_amd64_l4env_uclibc_minimal)
  LDSCRIPT_amd64_l4env_minimal    = $(LDSCRIPT_amd64_l4env_uclibc_minimal)
else
  LIBCINCDIR_amd64_l4env_minimal  = $(LIBCINCDIR_amd64_l4env_dietlibc_minimal)
  LIBCLIBDIR_amd64_l4env_minimal  = $(LIBCLIBDIR_amd64_l4env_dietlibc_minimal)
  L4LIBS_amd64_l4env_minimal      = $(L4LIBS_amd64_l4env_dietlibc_minimal)
  LIBCLIBS_amd64_l4env_minimal    = $(LIBCLIBS_amd64_l4env_dietlibc_minimal)
  CRT0_amd64_l4env_minimal        = $(CRT0_amd64_l4env_dietlibc_minimal)
  CRTN_amd64_l4env_minimal        = $(CRTN_amd64_l4env_dietlibc_minimal)
  LDSCRIPT_amd64_l4env_minimal    = $(LDSCRIPT_amd64_l4env_dietlibc_minimal)
endif

.general.d:        $(L4DIR_ABS)/mk/modes.inc
