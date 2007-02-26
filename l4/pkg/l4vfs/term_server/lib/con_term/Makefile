PKGDIR   ?= ../..
L4DIR    ?= $(PKGDIR)/../../..

SYSTEMS   = x86-l4v2 x86-l4x0

TARGET    = libcon_term.a
MODE      = l4env_minimal
CLIENTIDL = l4con.idl

# don't ask: it's a feature, not a bug
vpath l4con-client.c $(L4DIR)/pkg/l4con/idl/OBJ-$(ARCH)-$(L4API)

SRC_C     = open.c close.c evh.c helper.c color.c l4con-client.c cursor.c

include $(L4DIR)/mk/lib.mk
