NOTARGETSTOINSTALL = 1

PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
SYSTEMS          = x86-l4v2 x86-l4x0
TARGET           = libpresenter_util_presprov.a libpresenter_util_fprov.a

DEBUG		= 0

SRC_C_ALL	= keygenerator.c pool.c arraylist.c
SRC_C_libpresenter_util_presprov.a = $(SRC_C_ALL) dataprovider_presprov.c
SRC_C_libpresenter_util_fprov.a = $(SRC_C_ALL) dataprovider_fprov.c

CLIENTIDL_libpresenter_util_presprov.a  = presenter_fprov.idl

PRIVATE_INCDIR  = $(PKGDIR)/server/include/util 

include $(L4DIR)/mk/lib.mk
