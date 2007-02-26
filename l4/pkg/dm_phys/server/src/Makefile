PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dm_phys
DEFAULT_RELOC_x86 = 0x01500000
DEFAULT_RELOC_arm = 0x001b0000

MODE		= sigma0
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

LIBS		= -ldm_generic-server -ldm_generic -ldm_mem-server \
		  -lslab -levents -ll4env_err

SERVERIDL	= dm_phys.idl
SRC_C		= main.c sigma0.c kinfo.c internal_alloc.c memmap.c \
		  pages.c dataspace.c dataspace_iterate.c \
		  map.c open.c close.c size.c resize.c phys_addr.c \
		  lock.c clients.c transfer.c copy.c pagesize.c \
		  poolsize.c debug.c debug_dmphys.c events.c info.c
PRIVATE_INCDIR	= ../../include
CPPFLAGS	= -DDEBUG

include $(L4DIR)/mk/prog.mk


