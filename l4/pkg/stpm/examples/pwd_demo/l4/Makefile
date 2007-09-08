SYSTEMS         := x86-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = pwd pwd.bad
DEFAULT_RELOC    = 0x00a80000
LIBS             = -ll4dope -lloaderif

SRC_C_pwd        = pwd.c
SRC_C_pwd.bad    = pwd.bad.c

PRIVATE_INCDIR   = . \
                   $(PKGDIR)/include

DEFINES_pwd.c    = -DBAD_ONE=0
DEFINES_pwd.bad.c= -DBAD_ONE=1

vpath % $(PKGDIR)/examples/pwd_demo/common 

include $(L4DIR)/mk/prog.mk

