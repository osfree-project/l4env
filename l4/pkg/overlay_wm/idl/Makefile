PKGDIR       ?= ..
L4DIR        ?= $(PKGDIR)/../..

LINUX_OBJDIR ?= $(PKGDIR)/idl/OBJ-x86-linux
IDL_DIR      ?= $(PKGDIR)/idl

ifeq ($(L4API),linux)
MODE          = host
endif

SYSTEMS       = x86-linux x86-l4v2
PRIVATE_INCDIR = $(DICE_INCDIR)
IDL           = overlay.idl input_listener.idl window_listener.idl

IDL_EXPORT_STUB = overlay.idl input_listener.idl
IDL_EXPORT_SKELETON = input_listener.idl window_listener.idl overlay.idl
include $(L4DIR)/mk/idl.mk

