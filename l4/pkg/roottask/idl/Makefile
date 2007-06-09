PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(USE_TASKLIB),y)
IDL		= rmgr_task.idl
else
IDL 		= rmgr.idl
endif
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2 x86-l4secv2emu

include $(L4DIR)/mk/idl.mk
