PKGDIR      ?= ../../..
L4DIR       ?= $(PKGDIR)/../..

MODE         = host
TARGET       = testbed-client testbed-server
SRC_C_testbed-client = client.c
SRC_C_testbed-server = server.c
SYSTEMS      = x86-linux

PRIVATE_INCDIR = $(PKGDIR)/examples/testbed/linux
#PRIVATE_INCDIR = $(PKGDIR)/include \
#                 $(PKGDIR)/examples/idltest/include

vpath % $(PKGDIR)/examples/testbed/common

include $(L4DIR)/mk/prog.mk
