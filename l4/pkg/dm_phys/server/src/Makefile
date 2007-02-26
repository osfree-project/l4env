PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dm_phys
DEFAULT_RELOC	= 0x01500000
MODE		= oskit10_sigma0
SYSTEMS		= x86-l4v2 x86-l4x0

L4LIBS		= -ll4env_noenv -llogserver -lnames -ll4util \
		  -ldm_generic-server -lrmgr -lslab
LIBCLIBS	= -nostdlib -loskit10_support_tiny -loskit_c \
		  -loskit10_support_tiny

SERVERIDL	= dm_phys.idl
CRT0		= $(call findfile,crt0_l4env_noenv.o,$(L4LIBDIR))
SRC_C		= main.c sigma0.c kinfo.c internal_alloc.c memmap.c \
		  pages.c dataspace.c dataspace_iterate.c \
		  server.c map.c open.c close.c size.c resize.c phys_addr.c \
		  lock.c clients.c transfer.c copy.c pagesize.c \
		  debug.c debug_dmphys.c
PRIVATE_INCDIR	= ../../include
CPPFLAGS	= -DDEBUG

include $(L4DIR)/mk/prog.mk
