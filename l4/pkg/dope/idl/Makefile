PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../..

LINUX_OBJDIR ?= $(PKGDIR)/idl/OBJ-x86-linux
IDL_DIR      ?= $(PKGDIR)/idl

SYSTEMS       = x86-linux x86-l4v2 x86-l4x0

IDL           = dope.idl dopeapp.idl vscr.idl
IDL_FLAGS     = -fserver-parameter -fforce-corba-alloc -fctypes

IDL_EXPORT_SKELETON = dopeapp.idl

include $(L4DIR)/mk/idl.mk

