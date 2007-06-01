PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../..

IDL_DIR      ?= $(PKGDIR)/idl
SYSTEMS       = x86-l4v2 amd64-l4v2 arm-l4v2

IDL           = nitpicker.idl nitevent.idl
IDL_FLAGS     = -fforce-corba-alloc
IDL_EXPORT_SKELETON = nitevent.idl

include $(L4DIR)/mk/idl.mk

