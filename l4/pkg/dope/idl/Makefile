PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../..

LINUX_OBJDIR ?= $(PKGDIR)/idl/OBJ-x86-linux
IDL_DIR      ?= $(PKGDIR)/idl

SYSTEMS       = x86-linux x86-l4v2

IDL           = dope.idl dopeapp.idl vscr.idl
IDL_FLAGS     = -fforce-corba-alloc

IDL_EXPORT_SKELETON = dopeapp.idl

PRIVATE_INCDIR = /usr/include

include $(L4DIR)/mk/idl.mk

