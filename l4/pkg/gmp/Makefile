PKGDIR		?= .
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/subdir.mk

include: virt_dep

.PHONY: virt_dep
virt_dep:
	PWD=$(PWD)/lib/build $(MAKE) -C lib/build
