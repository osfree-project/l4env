PKGDIR		= .
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/subdir.mk

ifneq ($(strip $(GCCMAJORVERSION)),3)
$(error Wrong gcc major version "$(GCCMAJORVERSION)" - use 3.x for $(PKGNAME))
endif
