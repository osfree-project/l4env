PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

IDL 		= dm_mem.idl
IDL_FLAGS	= -fl4types
IDL_EXPORT_SKELETON = dm_mem.idl
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0
PRIVATE_INCDIR	= $(L4INCDIR)

include $(L4DIR)/mk/idl.mk
