# directories we need to know
PKGDIR		= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf.bid.local

ifeq ($(BUILD_lame),y)
TARGET += lame
endif
# it would be nice to build this conditionally, but this won't work, since
# file type probing (which is always done) uses this library
# ifeq ($(BUILD_mpg123),y)
TARGET += mpg123lib
# endif
ifneq ($(filter clean cleanall,$(MAKECMDGOALS)),)
TARGET += lame mpg123lib
endif

# include subdir role
include $(L4DIR)/mk/subdir.mk
