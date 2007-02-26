# -*- Makefile -*-
# vim:et:ts=4
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Include for defining the modes and architecture-specific flags

# we use these variables to define compiler-switches depending on the system
# we compile for. These variables are evaluated using the target-dependent
# variables ARCH and CPU
CARCHFLAGS_x86_586  = -march=i586
CARCHFLAGS_x86_686  = -march=i686
CARCHFLAGS_x86_K6   = -march=k6
CARCHFLAGS_x86_K7   = -march=athlon

# Default memory backend when linking against dietlibc or uClibc. The -u
# switch is necessary since mmap_anon is referenced using a weak symbol.
MALLOC_BACKEND      ?= -u mmap_anon -lc_be_simple_mem

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

# Chosable root lib, we just use the new libroot.o with dietlibc,
# if you want to change it, change it here (for you private setup)
ifeq ($(ARCH),x86)
 ifeq ($(USE_ROOTTASK_X86),y)
  ROOTLIB  = -lroot.o
  ROOTPLIB = -lroot.o.p
 else
  ROOTLIB  = -lrmgr
  ROOTPLIB = -lrmgr.p
 endif
else
 ROOTLIB  = -lroot.o
 ROOTPLIB = -lroot.o.p
endif


#######
#
# Mode definitions
#

#
# x86_sigma0_oskit06 - OSKit 0.6 and minimal environment (util, names, log)
#
BID_SUPPORTED_x86_sigma0_oskit06 = y
LIBCINCDIR_x86_sigma0_oskit06   = -nostdinc                                 \
                                  -I$(L4DIR)/../oskit                       \
                                  -I$(L4DIR)/../oskit/flux/c                \
                                  -I$(DROPS_STDDIR)/include/oskit06         \
                                  -I$(DROPS_STDDIR)/include/oskit06/flux/c  \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_sigma0_oskit06   = -L$(L4DIR)/../oskit/lib                   \
                                  -L$(DROPS_STDDIR)/lib/oskit06
LIBCLIBS_x86_sigma0_oskit06     = -nostdlib $(GCCLDNOSTDLIB) -lmc           \
                                  -loskit_support -ll4util $(ROOTLIB)       \
                                  -lkern -llmm -lmc $(GCCLIB) -lmc
L4LIBS_x86_sigma0_oskit06       = -static -lmain -llogserver -lnames        \
                                  -ll4util -ll4sys
CRT0_x86_sigma0_oskit06         = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_sigma0_oskit06         =
LDSCRIPT_x86_sigma0_oskit06     = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_sigma0_diet - sigma0 with dietlibc
#
BID_SUPPORTED_x86_sigma0_diet   = y
LIBCINCDIR_x86_sigma0_diet      = -nostdinc -DUSE_DIETLIBC=y                \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_sigma0_diet      =
LIBCLIBS_x86_sigma0_diet        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_minimal_io                      \
                                  -ldiet_be_l4_start_stop                   \
                                  -ldiet_be_sigma0_mem                      \
                                  -ldiet_c                                  \
                                  -llogserver -lnames -ll4util $(ROOTLIB)   \
                                  -ldiet_c $(GCCLIB) -ldiet_c
L4LIBS_x86_sigma0_diet          = -static -lmain -lnames                    \
                                  -llogserver -ll4util -ll4sys
CRT0_x86_sigma0_diet            = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_sigma0_diet            =
LDSCRIPT_x86_sigma0_diet        = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_sigma0_uc - sigma0 with uClibc
#
BID_SUPPORTED_x86_sigma0_uc     = y
LIBCINCDIR_x86_sigma0_uc        = -nostdinc                                 \
                                -I$(L4DIR)/include/x86/uclibc               \
                                -I$(L4DIR)/include/uclibc                   \
                                -I$(L4DIR)/include/uclibc++                 \
                                -I$(DROPS_STDDIR)/include/x86/uclibc        \
                                -I$(DROPS_STDDIR)/include/uclibc            \
                                -I$(DROPS_STDDIR)/include/uclibc++          \
                                -I$(GCCINCDIR)
LIBCLIBDIR_x86_sigma0_uc        =
LIBCLIBS_x86_sigma0_uc          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luc_be_minimal_io                        \
                                  -luc_be_l4_start_stop                     \
                                  -luc_be_sigma0_mem                        \
                                  -luc_c -luclibc_support                   \
                                  -llogserver -lnames -ll4util $(ROOTLIB)   \
                                  -luc_c $(GCCLIB) -luc_c
L4LIBS_x86_sigma0_uc            = $(L4LIBS_x86_sigma0_diet)
CRT0_x86_sigma0_uc              = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_sigma0_uc              =
LDSCRIPT_x86_sigma0_uc          = $(call findfile,main_stat.ld,$(L4LIBDIR))

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
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_x86_sigma0         = $(LIBCINCDIR_x86_sigma0_uc)
  LIBCLIBDIR_x86_sigma0         = $(LIBCLIBDIR_x86_sigma0_uc)
  LIBCLIBS_x86_sigma0           = $(LIBCLIBS_x86_sigma0_uc)
  L4LIBS_x86_sigma0             = $(L4LIBS_x86_sigma0_uc)
  CRT0_x86_sigma0               = $(CRT0_x86_sigma0_uc)
  CRTN_x86_sigma0               = $(CRTN_x86_sigma0_uc)
  LDSCRIPT_x86_sigma0           = $(LDSCRIPT_x86_sigma0_uc)
else
  LIBCINCDIR_x86_sigma0         = $(LIBCINCDIR_x86_sigma0_oskit06)
  LIBCLIBDIR_x86_sigma0         = $(LIBCLIBDIR_x86_sigma0_oskit06)
  LIBCLIBS_x86_sigma0           = $(LIBCLIBS_x86_sigma0_oskit06)
  L4LIBS_x86_sigma0             = $(L4LIBS_x86_sigma0_oskit06)
  CRT0_x86_sigma0               = $(CRT0_x86_sigma0_oskit06)
  CRTN_x86_sigma0               = $(CRTN_x86_sigma0_oskit06)
  LDSCRIPT_x86_sigma0           = $(LDSCRIPT_x86_sigma0_oskit06)
endif
endif


#
# x86_sima0_oskit10 - OSKit 1.0 and minimal environment (util, names, log)
#
BID_SUPPORTED_x86_sigma0_oskit10 = y
LIBCINCDIR_x86_sigma0_oskit10   = -nostdinc                                 \
                                  -I$(L4DIR)/../oskit10                     \
                                  -I$(L4DIR)/../oskit10/oskit/c             \
                                  -I$(DROPS_STDDIR)/include/oskit10         \
                                  -I$(DROPS_STDDIR)/include/oskit10/oskit/c \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_sigma0_oskit10   = -L$(L4DIR)/../oskit10/lib                 \
                                  -L$(DROPS_STDDIR)/lib/oskit10
LIBCLIBS_x86_sigma0_oskit10     = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -loskit10_support -loskit_startup         \
                                  -loskit_clientos -loskit_bootp            \
                                  -loskit_linux_fs -loskit_diskpart         \
                                  -loskit_linux_dev -loskit_freebsd_net     \
                                  -loskit10_support -loskit_kern            \
                                  -loskit_lmm -loskit_amm -loskit_c         \
                                  -loskit_kern $(ROOTLIB)                   \
                                  -loskit_c $(GCCLIB) -loskit_c
L4LIBS_x86_sigma0_oskit10       = -static -llogserver -lnames -ll4util      \
                                  $(ROOTLIB) -ll4sys
CRT0_x86_sigma0_oskit10         = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_sigma0_oskit10         = $(call findfile,crtn.o,$(L4LIBDIR))
LDSCRIPT_x86_sigma0_oskit10     = $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# all_libc - compiles C libs
#
BID_SUPPORTED_all_libc          = y
LIBCINCDIR_all_libc             = -nostdinc -I$(GCCINCDIR)                  \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc
LIBCLIBDIR_all_libc             =
LIBCLIBS_all_libc               = -nostdlib $(GCCLDNOSTDLIB) -ldiet_c       \
                                  $(LIBCBACKEND_LIB) $(GCCLIB) -ldiet_c
L4LIBS_all_libc                 =
CRT0_all_libc                   = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_all_libc                   =
LDSCRIPT_all_libc               = $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# all_plain - compilation for the host system
#
BID_SUPPORTED_all_plain         = y
LIBCINCDIR_all_plain            = -nostdinc -I$(GCCINCDIR)                  \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc
LIBCLIBDIR_all_plain            =
LIBCLIBS_all_plain              = -nostdlib $(GCCLDNOSTDLIB) $(GCCLIB) -ll4sys
L4LIBS_all_plain                =
CRT0_all_plain                  = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_all_plain                  =
LDSCRIPT_all_plain              = $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# x86_host - compilation for the host system
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
                                  -ll4util $(ROOTLIB) -lloaderif            \
                                  -ll4env -ll4env_err -lslab -ll4sys
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
# x86_l4env_oskit10 - OSKit 1.0 and l4env, the default
#
BID_SUPPORTED_x86_l4env_oskit10 = y
LIBCINCDIR_x86_l4env_oskit10    = -nostdinc -DOSKIT                         \
                                  -I$(L4DIR)/../oskit10                     \
                                  -I$(L4DIR)/../oskit10/oskit/c             \
                                  -I$(DROPS_STDDIR)/include/oskit10         \
                                  -I$(DROPS_STDDIR)/include/oskit10/oskit/c \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_oskit10    = -L$(L4DIR)/../oskit10/lib                 \
                                  -L$(DROPS_STDDIR)/lib/oskit10
LIBCLIBS_x86_l4env_oskit10      = -nostdlib $(GCCLDNOSTDLIB) -ll4env        \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -loskit_c -loskit_kern                    \
                                  -loskit_clientos -loskit_lmm              \
                                  -loskit_kern -loskit_c $(GCCLIB)          \
                                  -loskit_c
L4LIBS_x86_l4env_oskit10        = -static -loskit10_support_l4env.o         \
                                  -lgeneric_ts -ll4env -ll4rm               \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -lsemaphore -llogserver -lnames           \
                                  -ll4util $(ROOTLIB) -ll4env               \
                                  -ll4env_err -ll4rm -ldm_generic           \
                                  -ldm_mem -lthread -lslab -ll4sys
CRT0_x86_l4env_oskit10          = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_oskit10          =
LDSCRIPT_x86_l4env_oskit10      = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_l4env_diet - dietlibc and l4env, the default
#
BID_SUPPORTED_x86_l4env_diet    = y
LIBCINCDIR_x86_l4env_diet       = -nostdinc -DUSE_DIETLIBC=y                \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_diet       =
L4LIBS_x86_l4env_diet           = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule           \
                                  -lnames -ll4util $(ROOTLIB) -ll4env       \
                                  -ll4env_err -ll4rm -ldm_generic           \
                                  -ldm_mem -lthread -lslab -ll4sys
LIBCLIBS_x86_l4env_diet         = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap                               \
                                  -lc_be_mmap_util                          \
                                  -lc_be_l4env_start_stop                   \
                                  -lc_be_minimal_log_io                     \
                                  -ldiet_be_simple_sleep                    \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -lgeneric_ts -ldiet_c $(GCCLIB) -ldiet_c
CRT0_x86_l4env_diet             = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_diet             =
LDSCRIPT_x86_l4env_diet         = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_l4env_uc - uClibc and l4env, the default
#
BID_SUPPORTED_x86_l4env_uc      = y
LIBCINCDIR_x86_l4env_uc         = -nostdinc                                 \
                                  -I$(L4DIR)/include/x86/uclibc             \
                                  -I$(L4DIR)/include/uclibc                 \
                                  -I$(L4DIR)/include/uclibc++               \
                                  -I$(DROPS_STDDIR)/include/x86/uclibc      \
                                  -I$(DROPS_STDDIR)/include/uclibc          \
                                  -I$(DROPS_STDDIR)/include/uclibc++        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_uc         =
L4LIBS_x86_l4env_uc             = $(L4LIBS_x86_l4env_diet)
LIBCLIBS_x86_l4env_uc           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c                                    \
                                  -luclibc_support                          \
                                  $(MALLOC_BACKEND)                         \
                                  -lc_be_mmap                               \
                                  -lc_be_mmap_util                          \
                                  -lc_be_l4env_start_stop                   \
                                  -lc_be_minimal_log_io                     \
                                  -luc_be_simple_sleep                      \
                                  -ll4env -llogserver_capsule -ll4rm        \
                                  -ldm_generic -ldm_mem -lthread            \
                                  -lgeneric_ts -luc_c $(GCCLIB) -luc_c
CRT0_x86_l4env_uc               = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_uc               =
LDSCRIPT_x86_l4env_uc           = $(call findfile,main_stat.ld,$(L4LIBDIR))

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
ifeq ($(USE_UCLIBC),y)
  LIBCINCDIR_x86_l4env          = $(LIBCINCDIR_x86_l4env_uc)
  LIBCLIBDIR_x86_l4env          = $(LIBCLIBDIR_x86_l4env_uc)
  L4LIBS_x86_l4env              = $(L4LIBS_x86_l4env_uc)
  LIBCLIBS_x86_l4env            = $(LIBCLIBS_x86_l4env_uc)
  CRT0_x86_l4env                = $(CRT0_x86_l4env_uc)
  CRTN_x86_l4env                = $(CRTN_x86_l4env_uc)
  LDSCRIPT_x86_l4env            = $(LDSCRIPT_x86_l4env_uc)
else
  LIBCINCDIR_x86_l4env          = $(LIBCINCDIR_x86_l4env_oskit10)
  LIBCLIBDIR_x86_l4env          = $(LIBCLIBDIR_x86_l4env_oskit10)
  L4LIBS_x86_l4env              = $(L4LIBS_x86_l4env_oskit10)
  LIBCLIBS_x86_l4env            = $(LIBCLIBS_x86_l4env_oskit10)
  CRT0_x86_l4env                = $(CRT0_x86_l4env_oskit10)
  CRTN_x86_l4env                = $(CRTN_x86_l4env_oskit10)
  LDSCRIPT_x86_l4env            = $(LDSCRIPT_x86_l4env_oskit10)
endif
endif


#
# x86_l4env_dietlibc_minimal - dietlibc with just write(1, ...) as io backend
#
BID_SUPPORTED_x86_l4env_dietlibc_minimal = y
LIBCINCDIR_x86_l4env_dietlibc_minimal = -nostdinc -DUSE_DIETLIBC=y          \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
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
                                  -ldm_generic -lnames -ll4util             \
                                  $(DIETLIBC_IMPLEMENTATION) $(GCCLIB)      \
                                  $(DIETLIBC_IMPLEMENTATION)                \
                                  -lc_be_l4env_start_stop
L4LIBS_x86_l4env_dietlibc_minimal = -static -ll4rm -ldm_mem                 \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util $(ROOTLIB) -ll4sys
CRT0_x86_l4env_dietlibc_minimal = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_dietlibc_minimal =
LDSCRIPT_x86_l4env_dietlibc_minimal = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_l4env_uclibc_minimal - uClibc with just write(1, ...) as io backend
#
BID_SUPPORTED_x86_l4env_uclibc_minimal = y
LIBCINCDIR_x86_l4env_uclibc_minimal = -nostdinc                             \
                                  -I$(L4DIR)/include/x86/uclibc             \
                                  -I$(L4DIR)/include/uclibc                 \
                                  -I$(L4DIR)/include/uclibc++               \
                                  -I$(DROPS_STDDIR)/include/x86/uclibc      \
                                  -I$(DROPS_STDDIR)/include/uclibc          \
                                  -I$(DROPS_STDDIR)/include/uclibc++        \
                                  -I$(GCCINCDIR)
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
                                  -ldm_generic -lnames -ll4util             \
                                  $(UCLIBC_IMPLEMENTATION) $(GCCLIB)        \
                                  $(UCLIBC_IMPLEMENTATION)                  \
                                  -lc_be_l4env_start_stop -luclibc_support
L4LIBS_x86_l4env_uclibc_minimal = $(L4LIBS_x86_l4env_dietlibc_minimal)
CRT0_x86_l4env_uclibc_minimal   = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_uclibc_minimal   =
LDSCRIPT_x86_l4env_uclibc_minimal = $(call findfile,main_stat.ld,$(L4LIBDIR))

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
# x86_l4env_freebsd - huge OSKit 1.0 freebsd with tons of drivers and l4env
#
BID_SUPPORTED_x86_l4env_freebsd = y
LIBCINCDIR_x86_l4env_freebsd    = -nostdinc -DOSKIT                         \
                                  $(addprefix -I$(L4DIR)/,                  \
                                    ../oskit10/freebsd/libc/include         \
                                    ../oskit10/freebsd/3.x/src/include      \
                                    ../oskit10/freebsd/3.x/src/sys          \
                                    ../oskit10                              \
                                    include/c++/$(GCCVERSION)               \
                                    include/c++/$(GCCVERSION)/drops)        \
                                  $(addprefix -I$(DROPS_STDDIR)/include/,   \
                                    oskit10                                 \
                                    oskit10/oskit/freebsd                   \
                                    c++/$(GCCVERSION)                       \
                                    c++/$(GCCVERSION)/drops)                \
                                  -I$(GCCINCDIR)

LIBCLIBDIR_x86_l4env_freebsd    = -L$(L4DIR)/../oskit10/lib                 \
                                  -L$(L4DIR)/lib/c++/$(GCCVERSION)          \
                                  -L$(DROPS_STDDIR)/lib/oskit10             \
                                  -L$(DROPS_STDDIR)/lib/c++/$(GCCVERSION)
LIBCLIBS_x86_l4env_freebsd      = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -loskit_freebsd_c -loskit_freebsd_net     \
                                  -loskit_startup -loskit_clientos          \
                                  -loskit_bootp -loskit_linux_fs            \
                                  -loskit_fsnamespace -loskit_diskpart      \
                                  -loskit_linux_dev                         \
                                  -loskit10_support_l4env_freebsd_dev       \
                                  -loskit_kern -loskit_lmm                  \
                                  -loskit_amm -loskit_freebsd_c -loskit_com \
                                  $(GCCLIB) $(GCCLIB_EH) -loskit_freebsd_c
L4LIBS_x86_l4env_freebsd        = -static -loskit10_support_l4env_freebsd.o \
                                  -ll4env -ll4rm -ldm_generic               \
                                  -ldm_mem -lthread -lsemaphore -lnames     \
                                  -ll4util $(ROOTLIB) -ll4env -lomega0      \
                                  -llogserver -ll4env_err -ll4rm -ldm_generic \
                                  -ldm_mem -lthread -lslab -ll4sys
CRT0_x86_l4env_freebsd          = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_freebsd          = $(call findfile,crtn.o,$(L4LIBDIR))
LDSCRIPT_x86_l4env_freebsd      = $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# x86_l4env_base - dietlibc/uclibc with backends:
#     basic_io, io, basic_mmap, mmap_util, syslog, simple_sleep, time,
#     file_table
#
LIBCINCDIR_x86_l4env_base_diet  = -nostdinc -I$(L4DIR)/include/dietlibc     \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_base_diet  =
LIBCLIBS_x86_l4env_base_diet    = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -u printf -lc_be_io.o $(MALLOC_BACKEND)   \
                                  -lc_be_time -lrtc -ll4rm -ldm_mem         \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util $(DIETLIBC_IMPLEMENTATION)       \
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
                                  -lthread -ldm_generic -lnames -ll4util    \
                                  $(ROOTLIB) -ll4sys
CRT0_x86_l4env_base_diet        = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_base_diet        =
LDSCRIPT_x86_l4env_base_diet    = $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_x86_l4env_base_uc = y
LIBCINCDIR_x86_l4env_base_uc    = -nostdinc                                 \
                                  -I$(L4DIR)/include/x86/uclibc             \
                                  -I$(L4DIR)/include/uclibc                 \
                                  -I$(L4DIR)/include/uclibc++               \
                                  -I$(DROPS_STDDIR)/include/x86/uclibc      \
                                  -I$(DROPS_STDDIR)/include/uclibc          \
                                  -I$(DROPS_STDDIR)/include/uclibc++        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_base_uc    =
LIBCLIBS_x86_l4env_base_uc      = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -u printf -lc_be_io.o $(MALLOC_BACKEND)   \
                                  -lc_be_time -lrtc -ll4rm -ldm_mem         \
                                  -ldm_generic -lthread -lsemaphore -ll4env \
                                  -ll4env_err -lslab -llogserver_capsule    \
                                  -ll4rm -lthread -ldm_generic -lnames      \
                                  -ll4util $(UCLIBC_IMPLEMENTATION)         \
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
                                  -ll4vfs_extendable
L4LIBS_x86_l4env_base_uc        = -static -ll4rm -ldm_mem -ldm_generic      \
                                  -lthread -lsemaphore -ll4env -ll4env_err  \
                                  -lslab -llogserver_capsule -ll4rm         \
                                  -lthread -ldm_generic -lnames -ll4util    \
                                  $(ROOTLIB) -ll4sys
CRT0_x86_l4env_base_uc          = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_base_uc          =
LDSCRIPT_x86_l4env_base_uc      = $(call findfile,main_stat.ld,$(L4LIBDIR))

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
ifeq ($(USE_DIETLIBC),y)
  BID_SUPPORTED_x86_loader      = y
  LIBCINCDIR_x86_loader         = -nostdinc                                 \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
  LIBCLIBDIR_x86_loader         =
  LIBCLIBS_x86_loader           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c $(GCCLIB) -ldiet_c
  L4LIBS_x86_loader             = -lloader.s -ll4sys
  CRT0_x86_loader               = $(call findfile,crt0.o,$(L4LIBDIR))
  CRTN_x86_loader               =
  LDSCRIPT_x86_loader           = $(call findfile,main_stat.ld,$(L4LIBDIR))
else
ifeq ($(USE_UCLIBC),y)
  BID_SUPPORTED_x86_loader      = y
  LIBCINCDIR_x86_loader         = -nostdinc                                 \
                                  -I$(L4DIR)/include/x86/uclibc             \
                                  -I$(L4DIR)/include/uclibc                 \
                                  -I$(L4DIR)/include/uclibc++               \
                                  -I$(DROPS_STDDIR)/include/x86/uclibc      \
                                  -I$(DROPS_STDDIR)/include/uclibc          \
                                  -I$(DROPS_STDDIR)/include/uclibc++        \
                                  -I$(GCCINCDIR)
  LIBCLIBDIR_x86_loader         =
  LIBCLIBS_x86_loader           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c $(GCCLIB) -luc_c
  L4LIBS_x86_loader             = -lloader.s -ll4sys
  CRT0_x86_loader               = $(call findfile,crt0.o,$(L4LIBDIR))
  CRTN_x86_loader               =
  LDSCRIPT_x86_loader           = $(call findfile,main_stat.ld,$(L4LIBDIR))
else
  BID_SUPPORTED_x86_loader      = y
  LIBCINCDIR_x86_loader         = -nostdinc -DOSKIT                         \
                                  -I$(L4DIR)/../oskit10                     \
                                  -I$(L4DIR)/../oskit10/oskit/c             \
                                  -I$(DROPS_STDDIR)/include/oskit10         \
                                  -I$(DROPS_STDDIR)/include/oskit10/oskit/c \
                                  -I$(GCCINCDIR)
  LIBCLIBDIR_x86_loader         = -L$(L4DIR)/../oskit10/lib                 \
                                  -L$(DROPS_STDDIR)/lib/oskit10
  LIBCLIBS_x86_loader           = -nostdlib $(GCCLDNOSTDLIB) -loskit_c      \
                                  -loskit_kern -loskit_clientos -loskit_lmm \
                                  -loskit_c -loskit_kern -loskit_c
  L4LIBS_x86_loader             = -lloader.s -loskit10_support_l4env.o -ll4sys
  CRT0_x86_loader               = $(call findfile,crt0.o,$(L4LIBDIR))
  CRTN_x86_loader               =
  LDSCRIPT_x86_loader           = $(call findfile,main_stat.ld,$(L4LIBDIR))
endif
endif


#######################################################################
#
# sigma0 arm
#

BID_SUPPORTED_arm_sigma0_diet   = y
LIBCINCDIR_arm_sigma0_diet      = -nostdinc                                 \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_arm_sigma0_diet      =
LIBCLIBS_arm_sigma0_diet        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain                                    \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_l4_start_stop                   \
                                  -ldiet_be_minimal_io                      \
                                  -ldiet_be_sigma0_mem                      \
                                  $(GCCLIB) -ldiet_c
L4LIBS_arm_sigma0_diet          = -static -lmain -llogserver -lnames        \
                                  -ll4util -lroot.o -ll4sys
CRT0_arm_sigma0_diet            = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_arm_sigma0_diet            =
LDSCRIPT_arm_sigma0_diet        = $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_arm_sigma0_uc     = y
LIBCINCDIR_arm_sigma0_uc        = -nostdinc                                 \
                                  -I$(L4DIR)/include/arm/uclibc             \
                                  -I$(L4DIR)/include/uclibc                 \
                                  -I$(L4DIR)/include/uclibc++               \
                                  -I$(DROPS_STDDIR)/include/arm/uclibc      \
                                  -I$(DROPS_STDDIR)/include/uclibc          \
                                  -I$(DROPS_STDDIR)/include/uclibc++        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_arm_sigma0_uc        =
LIBCLIBS_arm_sigma0_uc          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain                                    \
                                  -luc_c                                    \
                                  -luclibc_support                          \
                                  -luc_be_l4_start_stop                     \
                                  -luc_be_minimal_io                        \
                                  -luc_be_sigma0_mem                        \
                                  $(GCCLIB) -luc_c
L4LIBS_arm_sigma0_uc            = $(L4LIBS_arm_sigma0_diet)
CRT0_arm_sigma0_uc              = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_arm_sigma0_uc              =
LDSCRIPT_arm_sigma0_uc          = $(call findfile,main_stat.ld,$(L4LIBDIR))

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
# Just a copy of sigma0 mode, so that not so many makefile changes
# are necessary
#

BID_SUPPORTED_arm_l4env_dietlibc_minimal    = $(BID_SUPPORTED_arm_sigma0_diet)
LIBCINCDIR_arm_l4env_dietlibc_minimal       = $(LIBCINCDIR_arm_sigma0_diet)
LIBCLIBDIR_arm_l4env_dietlibc_minimal       = $(LIBCLIBDIR_arm_sigma0_diet)
LIBCLIBS_arm_l4env_dietlibc_minimal         = $(LIBCLIBS_arm_sigma0_diet)
L4LIBS_arm_l4env_dietlibc_minimal           = $(L4LIBS_arm_sigma0_diet)
CRT0_arm_l4env_dietlibc_minimal             = $(CRT0_arm_sigma0_diet)
CRTN_arm_l4env_dietlibc_minimal             = $(CRTN_arm_sigma0_diet)
LDSCRIPT_arm_l4env_dietlibc_minimal         = $(LDSCRIPT_arm_sigma0_diet)

BID_SUPPORTED_arm_l4env_uclibc_minimal      = $(BID_SUPPORTED_arm_sigma0_uc)
LIBCINCDIR_arm_l4env_uclibc_minimal         = $(LIBCINCDIR_arm_sigma0_uc)
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
# Default mode
#

BID_SUPPORTED_arm_l4env         = y
LIBCINCDIR_arm_l4env_diet       = -nostdinc                                 \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_arm_l4env_diet       =
L4LIBS_arm_l4env_diet           = -static -lgeneric_ts -ll4env              \
                                  -ll4rm -ldm_generic -ldm_mem -lthread     \
                                  -lsemaphore -llogserver_capsule -lnames   \
                                  -ll4util -lroot.o -ll4env -ll4env_err     \
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
CRT0_arm_l4env_diet             = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_arm_l4env_diet             =
LDSCRIPT_arm_l4env_diet         = $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_arm_l4env_uc      = y
LIBCINCDIR_arm_l4env_uc         = -nostdinc                                 \
                                  -I$(L4DIR)/include/x86/uclibc             \
                                  -I$(L4DIR)/include/uclibc                 \
                                  -I$(L4DIR)/include/uclibc++               \
                                  -I$(DROPS_STDDIR)/include/x86/uclibc      \
                                  -I$(DROPS_STDDIR)/include/uclibc          \
                                  -I$(DROPS_STDDIR)/include/uclibc++        \
                                  -I$(GCCINCDIR)
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
CRT0_arm_l4env_uc               = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_arm_l4env_uc               =
LDSCRIPT_arm_l4env_uc           = $(call findfile,main_stat.ld,$(L4LIBDIR))

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

LIBCINCDIR_arm_loader_diet      = -nostdinc                                 \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_arm_loader_diet      =
LIBCLIBS_arm_loader_diet        = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -ldiet_c $(GCCLIB) -ldiet_c
L4LIBS_arm_loader_diet          = -lloader.s -ll4sys
CRT0_arm_loader_diet            = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_arm_loader_diet            =
LDSCRIPT_arm_loader_diet        = $(call findfile,main_stat.ld,$(L4LIBDIR))

LIBCINCDIR_arm_loader_uc        = -nostdinc                                 \
                                  -I$(L4DIR)/include/x86/uclibc             \
                                  -I$(L4DIR)/include/uclibc                 \
                                  -I$(L4DIR)/include/uclibc++               \
                                  -I$(DROPS_STDDIR)/include/x86/uclibc      \
                                  -I$(DROPS_STDDIR)/include/uclibc          \
                                  -I$(DROPS_STDDIR)/include/uclibc++        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_arm_loader_uc        =
LIBCLIBS_arm_loader_uc          = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -luc_c $(GCCLIB) -luc_c
L4LIBS_arm_loader_uc            = -lloader.s -ll4sys
CRT0_arm_loader_uc              = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_arm_loader_uc              =
LDSCRIPT_arm_loader_uc          = $(call findfile,main_stat.ld,$(L4LIBDIR))

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
LIBCLIBS_arm_l4linux            = -ldm_generic -ldm_mem -lnames -ll4util    \
                                  -lroot.o -lloaderif -ll4env -ll4env_err   \
                                  -lslab
CRT0_arm_l4linux                =
CRTN_arm_l4linux                =
LDSCRIPT_arm_l4linux            =


#######################################################################
#
# sigma0 amd64
#

BID_SUPPORTED_amd64_sigma0      = y
LIBCINCDIR_amd64_sigma0         = -nostdinc                                 \
                                  -I$(L4DIR)/include/dietlibc               \
                                  -I$(DROPS_STDDIR)/include/dietlibc        \
                                  -I$(GCCINCDIR)
LIBCLIBDIR_amd64_sigma0         =
LIBCLIBS_amd64_sigma0           = -nostdlib $(GCCLDNOSTDLIB)                \
                                  -lmain                                    \
                                  -ldiet_c                                  \
                                  -ldietlibc_support                        \
                                  -ldiet_be_l4_start_stop                   \
                                  -ldiet_be_minimal_io                      \
                                  -ldiet_be_sigma0_mem                      \
                                  $(GCCLIB) -ldiet_c
L4LIBS_amd64_sigma0             = -static -lmain -llogserver -lnames        \
                                  -ll4util -lroot.o
CRT0_amd64_sigma0               = $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_amd64_sigma0               =
LDSCRIPT_amd64_sigma0           = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# dietlibc minimal mode
#

BID_SUPPORTED_amd64_l4env_dietlibc_minimal  = $(BID_SUPPORTED_amd64_sigma0)
LIBCINCDIR_amd64_l4env_dietlibc_minimal     = $(LIBCINCDIR_amd64_sigma0)
LIBCLIBDIR_amd64_l4env_dietlibc_minimal     = $(LIBCLIBDIR_amd64_sigma0)
LIBCLIBS_amd64_l4env_dietlibc_minimal       = $(LIBCLIBS_amd64_sigma0)
L4LIBS_amd64_l4env_dietlibc_minimal         = $(L4LIBS_amd64_sigma0)
CRT0_amd64_l4env_dietlibc_minimal           = $(CRT0_amd64_sigma0)
CRTN_amd64_l4env_dietlibc_minimal           = $(CRTN_amd64_sigma0)
LDSCRIPT_amd64_l4env_dietlibc_minimal       = $(LDSCRIPT_amd64_sigma0)

#######################################################################
#
# IA64 
#

BID_SUPPORTED_ia64_sigma0       = y
LIBCINCDIR_ia64_sigma0          = -nostdinc $(addsuffix l4/libc/,           \
                                  $(L4INCDIR)) -I$(GCCINCDIR)
LIBCLIBDIR_ia64_sigma0          = 
LIBCLIBS_ia64_sigma0            = -nostdlib $(GCCLDNOSTDLIB) -lc
L4LIBS_ia64_sigma0              = -static -lnames -ll4util $(ROOTLIB)
CRT0_ia64_sigma0                = $(call findfile,crt0_main.o,$(L4LIBDIR))
CRTN_ia64_sigma0                =
LDSCRIPT_ia64_sigma0            = $(call findfile,main.ld,$(L4LIBDIR))

.general.d:        $(L4DIR)/mk/modes.inc
