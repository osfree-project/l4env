PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libl4con-idl.a
MODE		= l4env
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
CLIENTIDL	= l4con.idl stream.idl
SERVERIDL	= l4con.idl stream.idl
CPPFLAGS	+= -DL4_MAX_RPC_STRING_DOPES=3

include $(L4DIR)/mk/lib.mk
