PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= lib$(PKGNAME).a
SYSTEMS		= x86-l4v2

### specialties
GLOBAL_C	= global.c simple_strtoul.c
MM_C		= mm.c page.c address.c slab.c
USER_C		= wqueue.c init_task.c sched.c process.c workqueue.c
HMM_C		= ctype.c mmio.c mempool.c kobject.c kthread.c string.c \
		    base/map.c base/bus.c base/class.c base/class_simple.c base/core.c \
		    base/cpu.c base/dmapool.c base/driver.c base/firmware.c base/init.c \
		    base/interface.c base/platform.c base/sys.c

SRC_C		= $(MM_C) $(GLOBAL_C) $(USER_C) \
		  pci.c irq.c res.c softirq.c time.c $(HMM_C)

PRIVATE_INCDIR	= $(PKGDIR)/lib/include $(DDE26_INCDIR) $(LINUX26_INCDIR)

DEFINES		= -D__KERNEL__ -DDDE_LINUX
WARNINGS        = -Wall -Wstrict-prototypes

### act the role of ...
include $(L4DIR)/mk/lib.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif
