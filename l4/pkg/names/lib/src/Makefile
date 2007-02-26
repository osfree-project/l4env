#
# $Id$
#
# Makefile for sources of names library
#

PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS	= x86-l4v2 x86-l4x0

TARGET	= lib$(PKGNAME).a
MODE	= sigma0

CPPFLAGS += -I$(PKGDIR)/server/include

SRC_C	= libnames.c names_query_id.c names_query_name.c names_query_nr.c \
	  names_register.c names_unregister.c names_unregister_task.c \
	  names_waitfor_name.c

include $(L4DIR)/mk/lib.mk
