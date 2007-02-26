# directories we need to know
PKGDIR		= ../..
L4DIR		?= $(PKGDIR)/../..

-include $(PKGDIR)/Makeconf.bid.local

# no targets, we only use well known subdirs
ifeq ($(BUILD_lame),y)
TARGET += lame
endif
ifeq ($(BUILD_mpg123),y)
TARGET += mpg123lib
endif
ifneq ($(filter clean cleanall,$(MAKECMDGOALS)),)
TARGET += lame mpg123lib
endif

# include subdir role
include $(L4DIR)/mk/subdir.mk
