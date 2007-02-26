# 
# makefile for package IDL directory
#

PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

# create an idl file an assign its name to IDL
IDL	= ore_manager.idl ore_rxtx.idl ore_notify.idl
SYSTEMS = x86-l4v2

IDL_EXPORT_SKELETON = ore_notify.idl ore_manager.idl ore_rxtx.idl
# the default idl type is DCE/dice. Uncomment the next line to change this.
# IDL_TYPE= <your choice>

include $(L4DIR)/mk/idl.mk
