PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../../..

IDL	= vcontrol-remote.idl
IDL_TYPE = corba
IDL_FLAGS = -fctypes

-include $(PKGDIR)/../Makeconf.bid.local

ifeq ($(BUILD_RT_SUPPORT),y)
IDL_FLAGS += -DBUILD_RT_SUPPORT=1
endif

include $(L4DIR)/mk/idl.mk
