PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcon.a
MODE		= l4env
SYSTEMS		= x86-l4v2 x86-l4x0
CLIENTIDL	= con.idl
CPPFLAGS	+= -DL4_MAX_RPC_STRING_DOPES=3

include $(L4DIR)/mk/lib.mk
