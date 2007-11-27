PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

TARGET		= ldso_virtual_example
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
NO_DEFAULT_RELOC= 1
LDFLAGS		= --export-dynamic --dynamic-linker=libld-l4.s.so \
		  --rpath-link $(OBJ_BASE)/lib/$(ARCH)_$(CPU)/$(L4API)
LDSCRIPT	= $(PKGDIR)/examples/virtual/my_main_dyn.ld
LIBS		= -Bdynamic -ldl.s
L4LIBS		= -Bstatic -luclibc_support -lgeneric_ts -ll4env -ll4rm \
		  -ldm_generic -ldm_mem -lthread -lsemaphore -llogserver \
		  -lnames -ll4env -ll4env_err -ll4rm -ldm_generic \
		  -ldm_mem -lthread -lslab -ll4sys -ll4util_root -ll4util \
		  -lsigma0
LIBCLIBS	= -nostdlib -luc_c $(GCCLIB) -lc_be_simple_mem \
		  -lc_be_mmap -lc_be_mmap_util -ldm_mem \
		  -llogserver_capsule -ll4rm -luc_be_l4_start_stop

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
