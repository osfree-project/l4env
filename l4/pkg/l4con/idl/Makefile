PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..
IDL 		= l4con.idl stream.idl
IDL_FLAGS	= -fserver-parameter -fl4types
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

IDL_EXPORT_STUB	= stream.idl l4con.idl
IDL_EXPORT_SKELETON = stream.idl l4con.idl

include $(L4DIR)/mk/idl.mk
