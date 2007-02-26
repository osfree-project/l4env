PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../../..

IDL	= vcontrol-remote.idl
IDL_TYPE = corba

include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/../Makeconf.bid.local

ifeq ($(BUILD_RT_SUPPORT),y)
IDL_FLAGS += -DBUILD_RT_SUPPORT=1
endif

include $(L4DIR)/mk/idl.mk
