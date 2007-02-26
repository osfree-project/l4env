# 
# makefile for package IDL directory
#

PKGDIR ?= ..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2

# create an idl file an assign its name to IDL
IDL		= l4qws_qlock.idl l4qws_shm.idl
IDL_FLAGS	= -t

IDL_EXPORT_SKELETON = 

include $(L4DIR)/mk/idl.mk
