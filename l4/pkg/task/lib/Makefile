PKGDIR		= ..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf
ifeq ($(USE_TASKLIB),y)
TARGET		= client server
else
TARGET = 
endif

include $(L4DIR)/mk/subdir.mk
