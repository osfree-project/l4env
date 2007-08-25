PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

IDL 				= rmgr.idl
IDL_FLAGS-$(USE_TASKLIB)	+= -DUSE_TASKLIB
IDL_FLAGS			= $(IDL_FLAGS-y)
SYSTEMS				= x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu

include $(L4DIR)/mk/idl.mk
