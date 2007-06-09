PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifneq ($(USE_TASKLIB),y)
IDL 		= generic_ts.idl
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
IDL_EXPORT_SKELETON = generic_ts.idl
endif

include $(L4DIR)/mk/idl.mk
