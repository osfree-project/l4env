PKGDIR     ?= ../..
L4DIR      ?= $(PKGDIR)/../..

TARGET      = lib$(PKGNAME).a
SYSTEMS     = x86-l4v2

GLOBAL_C    = global.c
MM_C        = mm.c page.c address.c slab.c
USER_C      = wqueue.c sched.c process.c keventd.c
HMM_C       = mmio.c proc.c

SRC_C = $(MM_C) $(GLOBAL_C) $(USER_C) $(HMM_C) \
        pci.c irq.c softirq.c time.c res.c irq_stat.c bug.c ctor.c

PRIVATE_INCDIR = $(PKGDIR_ABS)/lib/include $(DDE_INCDIR) $(LINUX24_INCDIR)
LIBCINCDIR  = -nostdinc $(I_GCCINCDIR)

DEFINES     = -D__KERNEL__ -DDDE_LINUX
WARNINGS    = -Wall -Wstrict-prototypes -fno-strict-aliasing

include $(L4DIR)/mk/lib.mk
