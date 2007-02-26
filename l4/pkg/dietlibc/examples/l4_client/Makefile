# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
DEFAULT_RELOC	= 0x00980000
TARGET		= libc_libstdio_example
SYSTEMS		= x86-l4v2

MODE		= l4env_dietlibc_preliminary

###MODE		= l4env
###LIBCLIBS	= -nostdlib -ldiet_c -ldiet_be_simple -ll4rm -ldm_mem      \
###		  -ldm_generic -lthread -lsemaphore -ll4env -ll4env_err    \
###		  -lslab -llogserver -ll4rm -lthread -ldm_generic -lnames  \
###		  -ll4util -lrmgr -ldiet_c -ldiet_be_simple $(GCCLIB)
###LIBCINCDIR	=
####MODE		= host
###L4LIBS		=
###LIBS		= -ldiet_be_simple -uprintf

include $(L4DIR)/mk/prog.mk
