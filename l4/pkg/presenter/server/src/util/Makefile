NOTARGETSTOINSTALL = 1

PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
SYSTEMS          = x86-l4v2 x86-linux x86-l4x0
TARGET           = libutil.a

DEBUG		= 1

SRC_C	= keygenerator.c memory.c pool.c hashtab.c arraylist.c dataprovider.c

PRIVATE_INCDIR  = $(PKGDIR)/server/include/util 

include $(L4DIR)/mk/lib.mk
