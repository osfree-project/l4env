PKGDIR   ?= ..
L4DIR    ?= $(PKGDIR)/../../..

SYSTEMS   = x86-l4v2

IDL       = internal.idl main.idl

IDL_EXPORT_SKELETON = internal.idl main.idl

IDL_FLAGS = -t # -fforce-corba-alloc -ftrace-dump-msgbuf -ftrace-dump-msgbuf-dwords=4 -ftrace-function=LOG

include $(L4DIR)/mk/idl.mk
