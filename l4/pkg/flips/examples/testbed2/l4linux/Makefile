PKGDIR ?= ../../..
L4DIR  ?= $(PKGDIR)/../..

MODE     = l4linux
TARGET   = testbed2-lclient testbed2-lserver
SRC_C_testbed2-lclient = client.c
SRC_C_testbed2-lserver = server.c

vpath % $(PKGDIR)/examples/testbed2/common

include $(L4DIR)/mk/prog.mk
