PKGDIR		= .
L4DIR		?= $(PKGDIR)/../../..

TARGET =	idl

# verner config
-include $(PKGDIR)/../Makeconf.bid.local

ifeq "$(VCONREMOTE_BUILD)" "y"
TARGET +=	lib server
endif

include $(L4DIR)/mk/subdir.mk
