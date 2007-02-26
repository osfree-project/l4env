PKGDIR	= .
L4DIR	?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(filter y,$(BUILD_LOADER)),)
all idl cleanall ptest::
	@echo -e $(EMPHSTART)"  ==> ldso package is not built since BUILD_LOADER is disabled!\n      Use \"make config\" in L4DIR to activate!"$(EMPHSTOP)
else
include $(L4DIR)/mk/subdir.mk
endif
