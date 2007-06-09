PKGDIR		= ..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifneq ($(USE_TASKLIB),y)
TARGET		= clientlib serverlib
endif

include $(L4DIR)/mk/subdir.mk
