PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../..

IDL_DIR      ?= $(PKGDIR)/idl

SYSTEMS       = x86-l4v2 x86-l4x0

IDL           = nitpicker.idl nitevent.idl
IDL_FLAGS     = -fserver-parameter -fforce-corba-alloc -fctypes
IDL_EXPORT_SKELETON = nitevent.idl

include $(L4DIR)/mk/idl.mk

