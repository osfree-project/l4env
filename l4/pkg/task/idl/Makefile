PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf
ifeq ($(USE_TASKLIB),y)
IDL 		= generic_ts.idl
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
IDL_EXPORT_SKELETON = generic_ts.idl
else
IDL = 
endif

include $(L4DIR)/mk/idl.mk
