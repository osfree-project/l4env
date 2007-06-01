PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

CXXFLAGS_amd64-l4v2 += -mcmodel=medium
CFLAGS_amd64-l4v2 += -mcmodel=medium

MODE		= tiny
LIBCLIBS	= -nostdlib $(GCCLIB)
#LIBCINCDIR	= -nostdinc
SRC_CC		= io_backend.cc
SYSTEMS		= amd64-l4v2 x86-l4v2 arm-l4v2
TARGET		= libcxx_linux_io

include $(L4DIR)/pkg/cxx/lib/Makefile.inc
include $(L4DIR)/mk/lib.mk

