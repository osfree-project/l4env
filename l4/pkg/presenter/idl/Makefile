# 
# makefile for package IDL directory
#

PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

# create an idl file an assign its name to IDL
IDL	= presenter.idl
# the default idl type is DCE/dice. Uncomment the next line to change this.
# IDL_TYPE= <your choice>

IDL_FLAGS = -t

SYSTEMS = x86-l4v2 x86-l4x0

include $(L4DIR)/mk/idl.mk
