PKGDIR   = .
L4DIR   ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(LINUX24_DIR),)
  TARGET =
$(warning Flips not build. Set LINUX24_DIR to Linux 2.4 source tree to change that.)
endif

include $(L4DIR)/mk/subdir.mk

ifneq ($(strip $(GCCMAJORVERSION)),3)
$(error Wrong gcc major version "$(GCCMAJORVERSION)" - use 3.x for $(PKGNAME))
endif
