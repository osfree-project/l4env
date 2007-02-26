PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2

### specialties
GLOBAL_C	= global.c
MM_C		= mm.c page.c address.c slab.c
USER_C		= wqueue.c sched.c process.c
HMM_C		= mmio.c proc.c

SRC_C		= $(MM_C) $(GLOBAL_C) $(USER_C) $(HMM_C) \
		  pci.c irq.c softirq.c time.c res.c irq_stat.c bug.c

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
