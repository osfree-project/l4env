PKGDIR		= .
L4DIR		?= $(PKGDIR)/../../..

TARGET =	idl

# verner config
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/../Makeconf.bid.local

ifeq "$(VCONREMOTE_BUILD)" "y"
TARGET +=	lib server
endif

include $(L4DIR)/mk/subdir.mk
