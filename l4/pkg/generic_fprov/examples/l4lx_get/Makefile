PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2

MODE		= l4linux
TARGET		= l4lx_fprov_get
SRC_C		= main.c
LIBS		= -lloglinux -lgeneric_fprov -ll4env-l4lx

include $(L4DIR)/mk/prog.mk
