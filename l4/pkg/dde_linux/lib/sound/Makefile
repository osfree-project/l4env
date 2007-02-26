PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME)-sound.a
SYSTEMS		= x86-l4v2

### specialties
SRC_C		= soundcore.c helpers.c dummy.c
PRIVATE_INCDIR	= $(PKGDIR)/lib/include \
		  $(PKGDIR)/include/ARCH-$(ARCH) \
		  $(DDE_LINUX_DIR)/include

DEFINES		= -D__KERNEL__ -DDDE_LINUX
WARNINGS        = -Wall -Wstrict-prototypes

### act the role of ...
include $(L4DIR)/mk/lib.mk

ifeq ($(DDE_LINUX_DIR),)
  $(error Please set setup path to Linux 2.4.20 sources (DDE_LINUX_DIR) in $(L4DIR)/Makeconf.local)
endif

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif
