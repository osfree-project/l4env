# directories we need to know
PKGDIR  = .
L4DIR  ?= $(PKGDIR)/../..

# include subdir role
include $(L4DIR)/mk/subdir.mk

include: virt_dep

.PHONY: virt_dep
virt_dep:
	$(MAKE) -C lib/uclibc/
