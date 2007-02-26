# directories we need to know
PKGDIR		= ../../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

# no targets, we only use well known subdirs
ifeq ($(BUILD_ARCH),x86)
TARGET = build/generic
else
TARGET =
endif

# include subdir role
include $(L4DIR)/mk/subdir.mk
