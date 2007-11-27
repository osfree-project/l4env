# -*- Makefile -*-
# vim:et:ts=8:ft=make:
#
# Makefile-Include for defining modes

# Default memory backend when linking against uClibc. The -u
# switch is necessary since mmap_anon is referenced using a weak symbol.
MALLOC_BACKEND     ?= -u mmap_anon -lc_be_simple_mem

MMAP_BACKEND       ?= -lc_be_sigma0_mem

# Default is uClibc without reentrant functions. If, for example
# two independant threads of the same application call malloc at the same
# time, WANT_THREADSAFE _must_ be defined.
ifneq ("$(WANT_THREADSAFE)","")
UCLIBC_IMPLEMENTATION   := -luc_c_r
else
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

LIBC_INCLUDE   = -nostdinc                                 \
                 -I$(OBJ_BASE)/include/$(ARCH)/uclibc      \
                 -I$(OBJ_BASE)/include/uclibc              \
                 -I$(OBJ_BASE)/include/uclibc++            \
                 -I$(DROPS_STDDIR)/include/$(ARCH)/uclibc  \
                 -I$(DROPS_STDDIR)/include/uclibc          \
                 -I$(DROPS_STDDIR)/include/uclibc++        \
                 -I$(GCCINCDIR)
LIBC_LIB       = -luc_c

#############################################################################
#
# X86 (IA32)
#

#
# x86_tiny - minimal environment (only l4util, no log, no names)
#
BID_SUPPORTED_x86_tiny          = y
LIBCINCDIR_x86_tiny             = $(LIBC_INCLUDE)
LIBCLIBDIR_x86_tiny             =
LIBCLIBS_x86_tiny               = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support -ll4util -lsigma0 \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_x86_tiny                 = -static -lmain -ll4util -ll4sys
CRT0_x86_tiny                   = $(CRT0_DEFAULT)
CRTN_x86_tiny                   =
LDSCRIPT_x86_tiny               = $(LDS_DEFAULT)


#
# x86_sigma0 - minimal environment (util, names, log)
#
BID_SUPPORTED_x86_sigma0        = y
LIBCINCDIR_x86_sigma0           = $(LIBC_INCLUDE)
LIBCLIBDIR_x86_sigma0           =
LIBCLIBS_x86_sigma0             = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support                   \
                                  -llogserver -lnames -lsigma0              \
                                  -ll4util_root -ll4util $(ROOTLIB)         \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_x86_sigma0               = -static -lmain -lnames                    \
                                  -llogserver -ll4util_root -ll4util -ll4sys
CRT0_x86_sigma0                 = $(CRT0_DEFAULT)
CRTN_x86_sigma0                 =
LDSCRIPT_x86_sigma0             = $(LDS_DEFAULT)

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
BID_SUPPORTED_x86_l4env         = y
LIBCINCDIR_x86_l4env            = $(LIBC_INCLUDE)
LIBCLIBDIR_x86_l4env            =
L4LIBS_x86_l4env                = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule           \
                                  -lnames -ll4util_root -ll4util -lsigma0   \
                                  $(ROOTLIB) -ll4env -ll4env_err -ll4rm     \
                                  -ldm_generic -ldm_mem -lthread -lslab     \
                                  -ll4sys
LIBCLIBS_x86_l4env              = -nostdlib $(GCCLDNOSTDLIB)                \
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
CRT0_x86_l4env                  = $(CRT0_DEFAULT)
CRTN_x86_l4env                  =
LDSCRIPT_x86_l4env              = $(LDS_DEFAULT)

#
# x86_l4env_minimal - uClibc with just write(1, ...) as io backend
#
BID_SUPPORTED_x86_l4env_minimal = y
LIBCINCDIR_x86_l4env_minimal    = $(LIBC_INCLUDE)
LIBCLIBDIR_x86_l4env_minimal    =
LIBCLIBS_x86_l4env_minimal      = -nostdlib $(GCCLDNOSTDLIB)              \
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
L4LIBS_x86_l4env_minimal        = -static -ll4rm -ldm_mem                 \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4sys
CRT0_x86_l4env_minimal          = $(CRT0_DEFAULT)
CRTN_x86_l4env_minimal          =
LDSCRIPT_x86_l4env_minimal      = $(LDS_DEFAULT)

#
# x86_l4env_base - uclibc with backends:
#     basic_io, io, basic_mmap, mmap_util, syslog, simple_sleep, time,
#     file_table
#
BID_SUPPORTED_x86_l4env_base    = y
LIBCINCDIR_x86_l4env_base       = $(LIBC_INCLUDE)
LIBCLIBDIR_x86_l4env_base       =
LIBCLIBS_x86_l4env_base         = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -u printf -lc_be_io.o $(MALLOC_BACKEND)   \
                                  -lc_be_time -lrtc -ll4rm -ldm_mem         \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  $(GCCLIB)                                 \
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
                                  -ll4vfs_extendable -ll4sys                \
                                  $(UCLIBC_IMPLEMENTATION)
L4LIBS_x86_l4env_base           = -static -ll4rm -ldm_mem -ldm_generic      \
                                  -lthread -lsemaphore -ll4env -ll4env_err  \
                                  -lslab -llogserver_capsule -ll4rm         \
                                  -lthread -ldm_generic -lnames             \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4sys
CRT0_x86_l4env_base             = $(CRT0_DEFAULT)
CRTN_x86_l4env_base             =
LDSCRIPT_x86_l4env_base         = $(LDS_DEFAULT)

#
# x86_loader - applications to be loaded by the L4Env loader.
#              Will be merged with l4env_base and become the default mode.
#
BID_SUPPORTED_x86_loader      = y
LIBCINCDIR_x86_loader         = $(LIBC_INCLUDE)
LIBCLIBDIR_x86_loader         =
LIBCLIBS_x86_loader           = -nostdlib $(GCCLDNOSTDLIB)                \
                                -luc_c $(GCCLIB) -luc_c
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
BID_SUPPORTED_arm_tiny          = y
LIBCINCDIR_arm_tiny             = $(LIBC_INCLUDE)
LIBCLIBDIR_arm_tiny             =
LIBCLIBS_arm_tiny               = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain -luc_c -luclibc_support            \
                                  -luc_be_l4_start_stop                     \
                                  -luc_be_minimal_io                        \
                                  $(MMAP_BACKEND) -ll4util                  \
                                  -lsigma0 $(GCCLIB) -luc_c
L4LIBS_arm_tiny                 = -static -lmain -ll4util -ll4sys
CRT0_arm_tiny                   = $(CRT0_DEFAULT)
CRTN_arm_tiny                   =
LDSCRIPT_arm_tiny               = $(LDS_DEFAULT)

#
# sigma0
#
BID_SUPPORTED_arm_sigma0        = y
LIBCINCDIR_arm_sigma0           = $(LIBC_INCLUDE)
LIBCLIBDIR_arm_sigma0           =
LIBCLIBS_arm_sigma0             = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain                                    \
                                  -luc_c                                    \
                                  -luclibc_support                          \
                                  -luc_be_l4_start_stop                     \
                                  -luc_be_minimal_io                        \
                                  $(MMAP_BACKEND)                           \
                                  -lsigma0 $(GCCLIB) -luc_c
L4LIBS_arm_sigma0               = -static -lmain -llogserver -lnames        \
                                  -ll4util_root -ll4util $(ROOTLIB) -ll4sys
CRT0_arm_sigma0                 = $(CRT0_DEFAULT)
CRTN_arm_sigma0                 =
LDSCRIPT_arm_sigma0             = $(LDS_DEFAULT)

#
# arm_l4env_minimal
#
BID_SUPPORTED_arm_l4env_minimal = y
LIBCINCDIR_arm_l4env_minimal    = $(LIBC_INCLUDE)
LIBCLIBDIR_arm_l4env_minimal    =
LIBCLIBS_arm_l4env_minimal      = -nostdlib $(GCCLDNOSTDLIB)              \
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
L4LIBS_arm_l4env_minimal        = -static -ll4rm -ldm_mem                 \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4sys
CRT0_arm_l4env_minimal          = $(CRT0_DEFAULT)
CRTN_arm_l4env_minimal          =
LDSCRIPT_arm_l4env_minimal      = $(LDS_DEFAULT)

#
# arm_l4env -- default mode
#
BID_SUPPORTED_arm_l4env         = y
LIBCINCDIR_arm_l4env            = $(LIBC_INCLUDE)
LIBCLIBDIR_arm_l4env            =
L4LIBS_arm_l4env                = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule -lnames   \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4env -ll4env_err            \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lslab -ll4sys
LIBCLIBS_arm_l4env              = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c -luclibc_support                   \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap -lc_be_mmap_util              \
                                  -luc_be_l4_start_stop                     \
                                  -lc_be_minimal_log_io                     \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -luc_c $(GCCLIB) -luc_c
CRT0_arm_l4env                  = $(CRT0_DEFAULT)
CRTN_arm_l4env                  =
LDSCRIPT_arm_l4env              = $(LDS_DEFAULT)

BID_SUPPORTED_arm_l4env_base = y
LIBCINCDIR_arm_l4env_base    = $(LIBC_INCLUDE)
LIBCLIBDIR_arm_l4env_base    =
LIBCLIBS_arm_l4env_base      = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -u printf -lc_be_io.o $(MALLOC_BACKEND)   \
                                  -lc_be_time -lrtc -ll4rm -ldm_mem         \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  $(GCCLIB)                                 \
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
                                  -ll4vfs_extendable -ll4sys                \
                                  $(UCLIBC_IMPLEMENTATION)
L4LIBS_arm_l4env_base           = -static -ll4rm -ldm_mem -ldm_generic      \
                                  -lthread -lsemaphore -ll4env -ll4env_err  \
                                  -lslab -llogserver_capsule -ll4rm         \
                                  -lthread -ldm_generic -lnames             \
                                  -ll4util_root -ll4util -lsigma0           \
                                  $(ROOTLIB) -ll4sys
CRT0_arm_l4env_base          = $(CRT0_DEFAULT)
CRTN_arm_l4env_base          =
LDSCRIPT_arm_l4env_base      = $(LDS_DEFAULT)

#
# arm_loader - applications to be loaded by the L4Env loader.
#
BID_SUPPORTED_arm_loader        = y
LIBCINCDIR_arm_loader           = $(LIBC_INCLUDE)
LIBCLIBDIR_arm_loader           =
LIBCLIBS_arm_loader             = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c $(GCCLIB) -luc_c
L4LIBS_arm_loader               = -lloader.s -ll4sys
CRT0_arm_loader                 = $(CRT0_DEFAULT)
CRTN_arm_loader                 =
LDSCRIPT_arm_loader             = $(LDS_LOADER)

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
BID_SUPPORTED_amd64_tiny        = y
LIBCINCDIR_amd64_tiny           = $(LIBC_INCLUDE)
LIBCLIBDIR_amd64_tiny           =
LIBCLIBS_amd64_tiny             = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support -ll4util -lsigma0 \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_amd64_tiny               = -static -lmain -ll4util -ll4sys
CRT0_amd64_tiny                 = $(CRT0_DEFAULT)
CRTN_amd64_tiny                 =
LDSCRIPT_amd64_tiny             = $(LDS_DEFAULT)

#
# amd64_sigma0 - sigma0 mode
#
BID_SUPPORTED_amd64_sigma0      = y
LIBCINCDIR_amd64_sigma0         = $(LIBC_INCLUDE)
LIBCLIBDIR_amd64_sigma0         =
LIBCLIBS_amd64_sigma0           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  $(MMAP_BACKEND)                           \
                                  -luc_c -luclibc_support                   \
                                  -llogserver -lnames -lsigma0              \
                                  -ll4util_root -ll4util $(ROOTLIB)         \
                                  -luc_c $(GCCLIB) -luc_c -ll4sys
L4LIBS_amd64_sigma0             = -static -lmain -lnames                    \
                                  -llogserver -ll4util_root -ll4util -ll4sys
CRT0_amd64_sigma0               = $(CRT0_DEFAULT)
CRTN_amd64_sigma0               =
LDSCRIPT_amd64_sigma0           = $(LDS_DEFAULT)

#
# amd64_l4env - l4env, the default
#
BID_SUPPORTED_amd64_l4env       = y
LIBCINCDIR_amd64_l4env            = $(LIBC_INCLUDE)
LIBCLIBDIR_amd64_l4env            =
L4LIBS_amd64_l4env              = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule           \
                                  -lnames -ll4util_root -ll4util $(ROOTLIB) \
                                  -ll4env -ll4env_err -ll4rm -ldm_generic   \
                                  -ldm_mem -lthread -lslab -ll4sys
LIBCLIBS_amd64_l4env              = -nostdlib $(GCCLDNOSTDLIB)                \
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
CRT0_amd64_l4env                  = $(CRT0_DEFAULT)
CRTN_amd64_l4env                  =
LDSCRIPT_amd64_l4env              = $(LDS_DEFAULT)

BID_SUPPORTED_amd64_l4env_base = y
LIBCINCDIR_amd64_l4env_base    = $(LIBC_INCLUDE)
LIBCLIBDIR_amd64_l4env_base    =
LIBCLIBS_amd64_l4env_base      = -nostdlib $(GCCLDNOSTDLIB)                \
                               -u printf -lc_be_io.o $(MALLOC_BACKEND)   \
                               -lc_be_time -lrtc -ll4rm -ldm_mem         \
                               -ldm_generic -lthread -lsemaphore -ll4env \
                               -ll4env_err -lslab -llogserver_capsule    \
                               -ll4rm -lthread -ldm_generic -lnames      \
                               -ll4util_root -ll4util -lsigma0           \
                               $(UCLIBC_IMPLEMENTATION)                  \
                               $(GCCLIB)                                 \
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
                               -ll4vfs_extendable -ll4sys                \
                               $(UCLIBC_IMPLEMENTATION)
L4LIBS_amd64_l4env_base        = -static -ll4rm -ldm_mem -ldm_generic    \
                               -lthread -lsemaphore -ll4env -ll4env_err  \
                               -lslab -llogserver_capsule -ll4rm         \
                               -lthread -ldm_generic -lnames             \
                               -ll4util_root -ll4util -lsigma0           \
                               $(ROOTLIB) -ll4sys
CRT0_amd64_l4env_base         = $(CRT0_DEFAULT)
CRTN_amd64_l4env_base         =
LDSCRIPT_amd64_l4env_base     = $(LDS_DEFAULT)

#
# amd64_l4env_minimal - libc with just write(1, ...) as io backend
#
BID_SUPPORTED_amd64_l4env_minimal = y
LIBCINCDIR_amd64_l4env_minimal  = $(LIBC_INCLUDE)
LIBCLIBDIR_amd64_l4env_minimal  =
LIBCLIBS_amd64_l4env_minimal    = -nostdlib $(GCCLDNOSTDLIB)            \
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
                                  -lc_be_l4env_start_stop -ll4sys
L4LIBS_amd64_l4env_minimal      = -static -ll4rm -ldm_mem               \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util_root -ll4util $(ROOTLIB) -ll4sys
CRT0_amd64_l4env_minimal        = $(CRT0_DEFAULT)
CRTN_amd64_l4env_minimal        =
LDSCRIPT_amd64_l4env_minimal    = $(LDS_DEFAULT)

.general.d:        $(L4DIR_ABS)/mk/modes.inc
