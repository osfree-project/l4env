PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET          = libio-omega0.a
MODE		= l4env
INSTALL_TARGET  =
SYSTEMS         = x86-l4v2 x86-l4x0
SRC_C           = glue.c irq_threads.c globals.c server.c pic.c

### specialties
PRIVATE_INCDIR  = $(PKGDIR)/server/lib-omega0/include \
                  $(PKGDIR)/server/lib-omega0/include/omega0 \
                  $(L4DIR)/pkg/omega0/server/src
CFLAGS          = -fno-strict-aliasing

vpath %.c $(L4DIR)/pkg/omega0/server/src

include $(L4DIR)/mk/lib.mk
