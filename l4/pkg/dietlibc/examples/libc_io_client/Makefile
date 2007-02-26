# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
DEFAULT_RELOC	= 0x00980000
TARGET		= libc_io_test
SYSTEMS		= x86-l4v2

MODE		= l4env_dietlibc_minimal

LIBCLIBS	= -nostdlib                      \
		  -ldiet_c -ldiet_be_l4env_start_stop -ldiet_be_io         \
		  -ldiet_be_simple_mem                                     \
		  -ll4rm -ldm_mem -ldm_generic -lthread -lsemaphore        \
		  -ll4env -ll4env_err -lslab -llogserver -ll4rm -lthread   \
		  -ldm_generic -lnames -ll4util -lrmgr                     \
		  -ldiet_c $(GCCLIB) -ldiet_c                              \
		  -lbasic_name_server -lbasic_io -lname_server             \
		  -ldiet_be_l4env_start_stop

LIBS		= -lname_server -lbasic_name_server -lname_space_provider \
		  -lbasic_io -lattachable

include $(L4DIR)/mk/prog.mk
