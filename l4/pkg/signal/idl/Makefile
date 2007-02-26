# 
# makefile for package IDL directory
#

PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

# create an idl file an assign its name to IDL
IDL				= signal.idl
IDL_EXPORT_STUB	= signal.idl
IDL_FLAGS		+= -t

MODE			= l4env_base

# the default idl type is DCE/dice. Uncomment the next line to change this.
# IDL_TYPE= <your choice>

include $(L4DIR)/mk/idl.mk
