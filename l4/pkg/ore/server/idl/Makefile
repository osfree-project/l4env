# 
# makefile for package IDL directory
#

PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

# create an idl file an assign its name to IDL
IDL	= worker.idl
IDL_EXPORT_SKELETON = worker.idl
# the default idl type is DCE/dice. Uncomment the next line to change this.
# IDL_TYPE= <your choice>

IDL_FLAGS = -t

include $(L4DIR)/mk/idl.mk
