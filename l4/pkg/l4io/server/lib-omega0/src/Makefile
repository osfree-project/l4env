PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libomega0.a
SYSTEMS		= x86-l4v2
NOTARGETSTOINSTALL = 1
SRC_C		= glue.c irq_threads.c globals.c server.c pic.c

### specialties
PRIVATE_INCDIR	= $(PKGDIR)/server/lib-omega0/include \
		  $(PKGDIR)/server/lib-omega0/include/omega0 \
		  $(L4DIR)/pkg/omega0/server/src

vpath % $(L4DIR)/pkg/omega0/server/src

### act the role of ...
include $(L4DIR)/mk/lib.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif
