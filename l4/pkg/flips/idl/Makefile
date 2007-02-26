# 
# makefile for package IDL directory
#

PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

# create an idl file an assign its name to IDL
IDL = flips.idl
# the default idl type is DCE/dice. Uncomment the next line to change this.
# IDL_TYPE= <your choice>
IDL_FLAGS = -fserver-parameter -fforce-corba-alloc -fl4types

include $(L4DIR)/mk/idl.mk
