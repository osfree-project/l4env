PKGDIR         ?= ../../..
L4DIR          ?= $(PKGDIR)/../..

TARGET          = libio-omega0.a
MODE            = l4env
INSTALL_TARGET  =
SYSTEMS         = x86-l4v2 amd64-l4v2
SRC_C           = glue.c irq_threads.c globals.c server.c

PRIVATE_INCDIR  = $(PKGDIR)/server/lib-omega0/include \
                  $(L4DIR)/pkg/omega0/server/include \
                  $(L4DIR)/pkg/omega0/server/src
CFLAGS          = -fno-strict-aliasing

vpath %.c $(L4DIR)/pkg/omega0/server/src

include $(L4DIR)/mk/lib.mk
