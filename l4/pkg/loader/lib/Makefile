PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..
TARGET		= clientlib

include $(L4DIR)/mk/Makeconf

ifeq ($(BUILD_LOADER),y)
TARGET		+= runtime
endif

include $(L4DIR)/mk/subdir.mk

runtime: clientlib

