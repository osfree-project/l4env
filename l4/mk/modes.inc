# -*- Makefile -*-
#
# DROPS (Dresden Realtime OPerating System) Component
#
# Makefile-Include for defining the modes and architecture-specific flags

# we use these variables to define compiler-switches depending on the system
# we compile for. These variables are evaluated using the target-dependent
# variables ARCH and CPU
CARCHFLAGS_x86_586=-march=i586
CARCHFLAGS_x86_686=-march=i686
CARCHFLAGS_x86_K6=-march=k6
CARCHFLAGS_x86_K7=-march=K7

#######
#
# Mode definitions
#

#
# x86_sigma0 - OSKit 0.6 and minimal environment (util, names, log)
#
BID_SUPPORTED_x86_sigma0 = y
LIBCINCDIR_x86_sigma0	= -nostdinc $(addprefix -I,\
		$(L4DIR)/../oskit $(L4DIR)/../oskit/flux/c \
		$(DROPS_STDDIR)/include/oskit06 \
		$(DROPS_STDDIR)/include/oskit06/flux/c) \
		-I$(GCCINCDIR)
LIBCLIBDIR_x86_sigma0	= -L$(L4DIR)/../oskit/lib \
			  -L$(DROPS_STDDIR)/lib/oskit06
LIBCLIBS_x86_sigma0	= -nostdlib -lmc -loskit_support -lkern -llmm -lmc \
			  $(GCCLIB) -lmc
L4LIBS_x86_sigma0	= -static -lmain -llogserver -lnames -ll4util -lrmgr \
			  -ll4sys
CRT0_x86_sigma0		= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_sigma0		=
LDSCRIPT_x86_sigma0	= $(call findfile,main_stat.ld,$(L4LIBDIR))


BID_SUPPORTED_all_libc 	= y
LIBCINCDIR_all_libc	= -nostdinc -I$(GCCINCDIR) -I$(L4DIR)/include/dietlibc \
			  -I$(DROPS_STDDIR)/include/dietlibc 
LIBCLIBDIR_all_libc	=
LIBCLIBS_all_libc	= -nostdlib -ldiet_c $(LIBCBACKEND_LIB) \
			  $(GCCLIB) -ldiet_c
L4LIBS_all_libc		=
CRT0_all_libc		= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_all_libc		=
LDSCRIPT_all_libc	= $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# all_plain - compilation for the host system
#
BID_SUPPORTED_all_plain	= y
LIBCINCDIR_all_plain	= -nostdinc -I$(GCCINCDIR) -I$(L4DIR)/include/dietlibc \
			  -I$(DROPS_STDDIR)/include/dietlibc
LIBCLIBDIR_all_plain	=
LIBCLIBS_all_plain	= -nostdlib $(GCCLIB) -ll4sys
L4LIBS_all_plain	=
CRT0_all_plain		= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_all_plain		=
LDSCRIPT_all_plain	= $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# x86_host - compilation for the host system
#
BID_SUPPORTED_all_host	= y
LIBCINCDIR_all_host	=
LIBCLIBDIR_all_host	=
LIBCLIBS_all_host	=
L4LIBS_all_host		=
CRT0_all_host		=
CRTN_all_host		=
LDSCRIPT_all_host	=

#
# x86_l4linux - L4Env functions for L4Linux programs
#
BID_SUPPORTED_x86_l4linux = y
LIBCINCDIR_x86_l4linux	= -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4linux	= -L$(DROPS_STDDIR)/lib
LIBCLIBS_x86_l4linux	= -ldm_generic -ldm_mem -lnames -ll4util \
			  -lrmgr -lloaderif -ll4env -ll4env_err -lslab -ll4sys 
CRT0_x86_l4linux	=
CRTN_x86_l4linux	=
LDSCRIPT_x86_l4linux	=

#
# x86_l4linux_kern - build L4Linux kernel modules
#
BID_SUPPORTED_x86_l4linux_kern = y
LIBCINCDIR_x86_l4linux_kern = -nostdinc -I$(L4LINUX22_DIR)/include \
			      -D__KERNEL__ -DLINUX_ON_L4 -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4linux_kern =
LIBCLIBS_x86_l4linux_kern =
CRT0_x86_l4linux_kern	=
CRTN_x86_l4linux_kern	=
LDSCRIPT_x86_l4linux_kern =

#
# x86_oskit10_sima0 - OSKit 1.0 and minimal environment (util, names, log)
#
BID_SUPPORTED_x86_oskit10_sigma0 = y
LIBCINCDIR_x86_oskit10_sigma0 = -nostdinc $(addprefix -I, \
				 $(L4DIR)/../oskit10 \
				 $(L4DIR)/../oskit10/oskit/c \
				 $(DROPS_STDDIR)/include/oskit10 \
				 $(DROPS_STDDIR)/include/oskit10/oskit/c) \
				 -I$(GCCINCDIR)
LIBCLIBDIR_x86_oskit10_sigma0 = -L$(L4DIR)/../oskit10/lib \
				-L$(DROPS_STDDIR)/lib/oskit10
LIBCLIBS_x86_oskit10_sigma0   = -nostdlib -loskit10_support -loskit_startup \
				-loskit_clientos -loskit_bootp \
				-loskit_linux_fs -loskit_diskpart \
				-loskit_linux_dev -loskit_freebsd_net \
				-loskit10_support -loskit_kern -loskit_lmm \
				-loskit_amm -loskit_c -loskit_kern -lrmgr \
				-loskit_c $(GCCLIB) -loskit_c
L4LIBS_x86_oskit10_sigma0 = -static -llogserver -lnames -ll4util -lrmgr \
			    -ll4sys
CRT0_x86_oskit10_sigma0	= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_oskit10_sigma0	= $(call findfile,crtn.o,$(L4LIBDIR))
LDSCRIPT_x86_oskit10_sigma0 = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_l4env - OSKit 1.0 and l4env, the default
#
BID_SUPPORTED_x86_l4env	= y
LIBCINCDIR_x86_l4env	= -nostdinc -DOSKIT $(addprefix -I, \
			   $(L4DIR)/../oskit10 \
			   $(L4DIR)/../oskit10/oskit/c \
	                   $(DROPS_STDDIR)/include/oskit10 \
			   $(DROPS_STDDIR)/include/oskit10/oskit/c) \
			   -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env	= -L$(L4DIR)/../oskit10/lib \
			  -L$(DROPS_STDDIR)/lib/oskit10
LIBCLIBS_x86_l4env	= -nostdlib -ll4env -ll4rm \
			  -ldm_generic -ldm_mem -lthread -loskit_c \
			  -loskit_kern -loskit_clientos -loskit_lmm \
			  -loskit_kern -loskit_c $(GCCLIB) -loskit_c
L4LIBS_x86_l4env	= -static -loskit10_support_l4env.o \
			  -ll4env -ll4rm -ldm_generic -ldm_mem \
			  -lthread -lsemaphore -llogserver -lnames -ll4util \
			  -lrmgr -ll4env -ll4env_err -ll4rm \
			  -ldm_generic -ldm_mem -lthread -lslab -ll4sys
CRT0_x86_l4env		= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env		=
LDSCRIPT_x86_l4env	= $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_l4env_freebsd - huge OSKit 1.0 freebsd with tons of drivers and l4env
#
BID_SUPPORTED_x86_l4env_freebsd = y
LIBCINCDIR_x86_l4env_freebsd = -nostdinc -DOSKIT $(addprefix -I, \
			   $(L4DIR)/../oskit10/freebsd/libc/include \
			   $(L4DIR)/../oskit10/freebsd/3.x/src/include \
			   $(L4DIR)/../oskit10/freebsd/3.x/src/sys \
			   $(L4DIR)/../oskit10 \
			   $(L4DIR)/include/c++/$(GCCVERSION) \
			   $(L4DIR)/include/c++/$(GCCVERSION)/drops \
			   $(DROPS_STDDIR)/include/oskit10 \
			   $(DROPS_STDDIR)/include/oskit10/oskit/freebsd \
			   $(DROPS_STDDIR)/include/c++/$(GCCVERSION) \
			   $(DROPS_STDDIR)/include/c++/$(GCCVERSION)/drops) \
			   -I$(GCCINCDIR)

LIBCLIBDIR_x86_l4env_freebsd = -L$(L4DIR)/../oskit10/lib \
			       -L$(L4DIR)/lib/c++/$(GCCVERSION) \
			       -L$(DROPS_STDDIR)/lib/oskit10 \
			       -L$(DROPS_STDDIR)/lib/c++/$(GCCVERSION)
LIBCLIBS_x86_l4env_freebsd = -nostdlib \
			  -loskit_freebsd_c -loskit_freebsd_net \
			  -loskit_startup -loskit_clientos -loskit_bootp \
			  -loskit_linux_fs \
			  -loskit_fsnamespace -loskit_diskpart \
			  -loskit_linux_dev \
			  -loskit10_support_l4env_freebsd_dev \
			  -loskit_kern -loskit_lmm \
			  -loskit_amm -loskit_freebsd_c -loskit_com \
			  $(GCCLIB) $(GCCLIB_EH) -loskit_freebsd_c
L4LIBS_x86_l4env_freebsd = -static -loskit10_support_l4env_freebsd.o \
			  -ll4env -ll4rm -ldm_generic \
			  -ldm_mem -lthread -lsemaphore -lnames \
			  -ll4util -lrmgr -ll4env -lomega0 -llogserver \
			  -ll4env_err -ll4rm -ldm_generic -ldm_mem -lthread \
			  -lslab -ll4sys
CRT0_x86_l4env_freebsd	= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_freebsd	= $(call findfile,crtn.o,$(L4LIBDIR))
LDSCRIPT_x86_l4env_freebsd = $(call findfile,main_stat.ld,$(L4LIBDIR))

#
# x86_l4env_base - dietlibc with l4env functionality. Default candidate.
#
BID_SUPPORTED_x86_l4env_base	= y
LIBCINCDIR_x86_l4env_base = -nostdinc					\
			    -I$(L4DIR)/include/dietlibc			\
			    -I$(DROPS_STDDIR)/include/dietlibc          \
			    -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_base =
LIBCLIBS_x86_l4env_base	= -nostdlib                      		\
			  -ldiet_be_l4env_base.o -ldiet_c -ll4rm -ldm_mem	\
			  -ldm_generic -lthread -lsemaphore -ll4env	\
			  -ll4env_err -lslab -llogserver -ll4rm		\
			  -lthread -ldm_generic -lnames -ll4util -lrmgr \
			  -ldiet_c $(GCCLIB) -ldiet_c
L4LIBS_x86_l4env_base	= -static -ll4rm -ldm_mem -ldm_generic -lthread	\
			   -lsemaphore -ll4env -ll4env_err -lslab	\
			   -llogserver -ll4rm -lthread -ldm_generic	\
			   -lnames  -ll4util -lrmgr

CRT0_x86_l4env_base	= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_base	=
LDSCRIPT_x86_l4env_base	= $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# x86_l4env_dietlibc_preliminary - dietlibc with L4Linux Posix backend
#
BID_SUPPORTED_x86_l4env_dietlibc_preliminary	= y
LIBCINCDIR_x86_l4env_dietlibc_preliminary	= -nostdinc              \
		-I$(L4DIR)/include/dietlibc                              \
		-I$(DROPS_STDDIR)/include/dietlibc                       \
		-I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_dietlibc_preliminary	=
LIBCLIBS_x86_l4env_dietlibc_preliminary	= -nostdlib                      \
		-ldiet_c -ldiet_be_l4env_start_stop -ldiet_be_l4xvfs     \
		-ldiet_be_simple_mem                                     \
		-ll4rm -ldm_mem -ldm_generic                             \
		-lthread -lsemaphore -ll4env -ll4env_err -lslab          \
		-llogserver -ll4rm -lthread -ldm_generic -lnames         \
		-ll4util -lrmgr                                          \
		-ldiet_c $(GCCLIB) -ldiet_c
L4LIBS_x86_l4env_dietlibc_preliminary	= -static -ll4rm -ldm_mem        \
		-ldm_generic -lthread -lsemaphore -ll4env -ll4env_err    \
		-lslab -llogserver -ll4rm -lthread -ldm_generic -lnames  \
		-ll4util -lrmgr
CRT0_x86_l4env_dietlibc_preliminary	= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_dietlibc_preliminary	=
LDSCRIPT_x86_l4env_dietlibc_preliminary	= $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# x86_l4env_dietlibc_minimal - dietlibc with just write(1, ...) as io backend
#
BID_SUPPORTED_x86_l4env_dietlibc_minimal	= y
LIBCINCDIR_x86_l4env_dietlibc_minimal		= -nostdinc              \
		-I$(L4DIR)/include/dietlibc                              \
		-I$(DROPS_STDDIR)/include/dietlibc -I$(GCCINCDIR)
LIBCLIBDIR_x86_l4env_dietlibc_minimal	=
LIBCLIBS_x86_l4env_dietlibc_minimal	= -nostdlib                      \
		-ldiet_c -ldiet_be_l4env_start_stop -ldiet_be_minimal_io \
		-ldiet_be_simple_mem                                     \
		-ll4rm -ldm_mem -ldm_generic -lthread -lsemaphore        \
		-ll4env -ll4env_err -lslab -llogserver -ll4rm -lthread   \
		-ldm_generic -lnames -ll4util -lrmgr                     \
		-ldiet_c $(GCCLIB) -ldiet_c
L4LIBS_x86_l4env_dietlibc_minimal	= -static -ll4rm -ldm_mem        \
		-ldm_generic -lthread -lsemaphore -ll4env -ll4env_err    \
		-lslab -llogserver -ll4rm -lthread -ldm_generic -lnames  \
		-ll4util -lrmgr
CRT0_x86_l4env_dietlibc_minimal	= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_l4env_dietlibc_minimal	=
LDSCRIPT_x86_l4env_dietlibc_minimal	= $(call findfile,main_stat.ld,$(L4LIBDIR))


#
# x86_loader - applications to be loaded by the L4Env loader.
#              Will be merged with l4env_base and become the default mode.
#
BID_SUPPORTED_x86_loader = y
LIBCINCDIR_x86_loader	= -nostdinc -DOSKIT $(addprefix -I, \
			   $(L4DIR)/../oskit10 \
			   $(L4DIR)/../oskit10/oskit/c \
	                   $(DROPS_STDDIR)/include/oskit10 \
			   $(DROPS_STDDIR)/include/oskit10/oskit/c) \
			   -I$(GCCINCDIR)
LIBCLIBDIR_x86_loader	= -L$(L4DIR)/../oskit10/lib \
			  -L$(DROPS_STDDIR)/lib/oskit10
LIBCLIBS_x86_loader	= -nostdlib -loskit_c -loskit_kern -loskit_clientos \
			  -loskit_lmm -loskit_c -loskit_kern -loskit_c
L4LIBS_x86_loader	= -lloader.s -loskit10_support_l4env.o \
			  -ll4sys
CRT0_x86_loader		= $(call findfile,crt0.o,$(L4LIBDIR))
CRTN_x86_loader		=
LDSCRIPT_x86_loader	= $(call findfile,main_stat.ld,$(L4LIBDIR))

BID_SUPPORTED_ia64_sigma0= y
LIBCINCDIR_ia64_sigma0	= -nostdinc $(addsuffix l4/libc/, $(L4INCDIR)) \
			  -I$(GCCINCDIR)
LIBCLIBDIR_ia64_sigma0	= 
LIBCLIBS_ia64_sigma0	= -nostdlib -lc
L4LIBS_ia64_sigma0	= -static -lnames -ll4util -lrmgr 
CRT0_ia64_sigma0	= $(call findfile,crt0_main.o,$(L4LIBDIR))
CRTN_ia64_sigma0	=
LDSCRIPT_ia64_sigma0	= $(call findfile,main.ld,$(L4LIBDIR))

.general.d:	$(L4DIR)/mk/modes.inc
