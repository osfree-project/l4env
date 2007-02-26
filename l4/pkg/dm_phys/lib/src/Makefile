PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libdm_phys.a
MODE		= sigma0
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

SRC_C		= dm_phys.c open.c copy.c pagesize.c poolsize.c \
		  debug_dmphys.c
CLIENTIDL	= dm_phys.idl
PRIVATE_INCDIR	= ../../include

include $(L4DIR)/mk/lib.mk
