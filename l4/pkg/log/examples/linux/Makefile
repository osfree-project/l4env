#
# $Id$
#

SYSTEMS:= x86
PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET  = linux-log
MODE    = linux
SRC_C   = main.c
LIBS	= -lloglinux

PRIVATE_INCDIR = $(L4INCDIR)
INSTALL_TARGET	= 
LOGTAG  = $(addprefix ",$(addsuffix ",$(TARGET)))     

CPPFLAGS+= -D__L4__ -DLOG_TAG='$(LOGTAG)'

include $(L4DIR)/mk/prog.mk
