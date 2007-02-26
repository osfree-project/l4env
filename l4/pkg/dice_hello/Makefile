PKGDIR	?= .
L4DIR	?= $(PKGDIR)/../..

TARGET	= idl client server

TEST_DEPENDS = server client

ifneq ($(filter clean cleanall,$(MAKECMDGOALS)),)
TARGET += ptest
endif

include $(L4DIR)/mk/subdir.mk
