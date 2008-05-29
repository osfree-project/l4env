PKGDIR		         ?= ..
L4DIR              ?= $(PKGDIR)/../..

IDL                 = generic_fprov.idl fprov_ext.idl
SYSTEMS             = x86-l4v2 arm-l4v2 amd64-l4v2
IDL_EXPORT_SKELETON = generic_fprov.idl fprov_ext.idl
IDL_FLAGS           = -p l4/generic_fprov

include $(L4DIR)/mk/idl.mk
