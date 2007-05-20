PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

TARGET		= ldso_virtual_example
SYSTEMS		= x86-l4v2 arm-l4x0
LDFLAGS		= --export-dynamic --dynamic-linker=libld-l4.s.so \
		  --rpath-link $(OBJ_BASE)/lib/$(ARCH)_$(CPU)/$(L4API)
LDSCRIPT	= $(PKGDIR)/examples/virtual/my_main_dyn.ld
LIBS		= -Bdynamic -ldl.s
L4LIBS		= -Bstatic $(L4LIBS_$(LIBC_TYPE)) -lgeneric_ts -ll4env -ll4rm \
		  -ldm_generic -ldm_mem -lthread -lsemaphore -llogserver \
		  -lnames -ll4env -ll4env_err -ll4rm -ldm_generic \
		  -ldm_mem -lthread -lslab -ll4sys -ll4util_root -ll4util \
		  -lsigma0
LIBCLIBS_diet	= -nostdlib -ldiet_c $(GCCLIB) -lc_be_simple_mem \
		  -lc_be_mmap -lc_be_mmap_util -ldm_mem \
		  -llogserver_capsule -ll4rm -ldiet_be_l4_start_stop
LIBCLIBS_uc	= -nostdlib -luc_c $(GCCLIB) -lc_be_simple_mem \
		  -lc_be_mmap -lc_be_mmap_util -ldm_mem \
		  -llogserver_capsule -ll4rm -luc_be_l4_start_stop

L4LIBS_diet	= -ldietlibc_support
L4LIBS_uc	= -luclibc_support

LIBCLIBS	= $(LIBCLIBS_$(LIBC_TYPE))

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
