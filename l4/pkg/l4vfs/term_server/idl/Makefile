PKGDIR   ?= ..
L4DIR    ?= $(PKGDIR)/../../..

SYSTEMS   = x86-l4v2 x86-l4x0

IDL       = internal.idl main.idl

IDL_EXPORT_SKELETON = internal.idl main.idl

IDL_FLAGS = -t -fl4types -fserver-parameter # -fforce-corba-alloc -ftrace-dump-msgbuf -ftrace-dump-msgbuf-dwords=4 -ftrace-function=LOG

include $(L4DIR)/mk/idl.mk
