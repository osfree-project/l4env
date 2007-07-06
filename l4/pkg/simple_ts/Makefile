PKGDIR		= .
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(USE_TASKLIB),y)
idl include src lib server doc examples clean cleanall:
else
include $(L4DIR)/mk/subdir.mk
endif
