PKGDIR   = .
L4DIR   ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(LINUX24_DIR),)
  TARGET =
$(warning Flips not build. Set LINUX24_DIR to Linux 2.4.34.4 source tree to change that.)
endif
#ifeq ($(LINUX26_DIR),)
#  TARGET =
#$(warning Flips not build. Set LINUX26_DIR to Linux 2.6.20.19 source tree to change that.)
#endif

include $(L4DIR)/mk/subdir.mk
