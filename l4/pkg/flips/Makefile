PKGDIR   = .
L4DIR   ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(LINUX24_DIR),)
  TARGET =
endif

include $(L4DIR)/mk/subdir.mk

# We only build for x86 anyway
ifneq ($(strip $(GCCMAJORVERSION_x86)),3)
$(error Wrong gcc major version "$(GCCMAJORVERSION_x86)" - use 3.x for $(PKGNAME))
endif
