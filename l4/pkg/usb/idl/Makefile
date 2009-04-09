# 
# makefile for package IDL directory
#

PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

# create an idl file an assign its name to IDL
IDL	= core.idl gadget.idl
SYSTEMS = x86-l4v2

IDL_EXPORT_SKELETON = core.idl
# the default idl type is DCE/dice. Uncomment the next line to change this.
# IDL_TYPE= <your choice>
#IDL_FLAGS =  -fconst-as-define -P-I$(OBJ_BASE)/include/$(ARCH)/l4/dde/linux26/linux-headers \
                    -P-I$(OBJ_BASE)/include/l4/dde/linux26/linux-headers \
	     -P-D__KERNEL__ -P-D_LINUX_STDDEF_H -P-D__DICE__
 
include $(L4DIR)/mk/idl.mk
