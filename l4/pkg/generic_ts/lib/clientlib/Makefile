PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

CLIENTIDL	= $(PKGNAME).idl
SRC_C		= server.c open.c free.c exit.c kill.c convert.c dump.c \
		  owner.c

include $(L4DIR)/mk/lib.mk
