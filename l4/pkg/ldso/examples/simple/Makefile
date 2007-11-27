PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SUBDIRS		= libsimple1_dyn libsimple2_dyn
all:: $(SUBDIRS)

include $(L4DIR)/mk/Makeconf

TARGET		= ldso_simple_example
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
NO_DEFAULT_RELOC= 1
LDFLAGS		= --no-undefined \
		  --dynamic-linker libld-l4.s.so \
		  --export-dynamic
LDSCRIPT	= $(call findfile,main_dyn.ld,$(L4LIBDIR))
LIBS		= -Bdynamic -lsimple1_dyn.s -lsimple2_dyn.s
L4LIBS		= -Bstatic -lgeneric_ts -ll4env \
		  -ll4rm -ldm_generic -ldm_mem -lthread -lsemaphore \
		  -llogserver -lnames $(ROOTLIB) -ll4env -ll4env_err -ll4rm \
		  -ldm_generic -ldm_mem -lthread -lslab -ll4util_root \
		  -ll4util -lsigma0 -ll4sys
LIBCLIBS	= -nostdlib -luc_c $(GCCLIB) -lc_be_sigma0_mem \
		  -luc_be_l4_start_stop

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
