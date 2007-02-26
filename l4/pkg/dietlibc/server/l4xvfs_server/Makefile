# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		l4xvfs_server_impl.c
#PRIVATE_INCDIR = ../../include
SERVERIDL =	l4xvfs.idl

# target
TARGET =	l4xvfs_server
SYSTEMS = 	x86-l4v2
MODE = 		l4linux
L4LIBS = 	

DEBUG		= 1

include $(L4DIR)/mk/prog.mk

# we do not need other L4 libs than this
LIBCLIBS_x86_l4linux = -lnames -lrmgr
