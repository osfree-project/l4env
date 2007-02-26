# directories we need to know
PKGDIR		= ../..
L4DIR		?= $(PKGDIR)/../..

-include $(PKGDIR)/Makeconf.bid.local

# build goom ? 
ifeq ($(BUILD_goom),y)
TARGET += goom
endif

# include subdir role
include $(L4DIR)/mk/subdir.mk
