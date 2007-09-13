PKGDIR     ?= ../../..
L4DIR      ?= $(PKGDIR)/../..
CONTRIB    ?= $(PKGDIR)/linux26/contrib

-include $(PKGDIR_OBJ)/Makeconf

ifeq ($(CONFIG_DDE26_COMMON),y)
TARGET += libdde_linux26.a
endif

ifeq ($(CONFIG_DDE26_NET),y)
TARGET += libdde_linux26_net.a
endif

ifeq ($(CONFIG_DDE26_BLOCK),y)
TARGET += libdde_linux26_block.a
endif

SYSTEMS = x86-l4v2

ifeq ($(ARCH), x86)
ARCH_DIR = arch/i386
endif

# contrib sources are in $(CONTRIB)
vpath %.c     $(CONTRIB)
vpath %.S    $(CONTRIB)

PRIVATE_INCDIR += $(CONTRIB)/drivers/pci $(PKGDIR)/linux26/lib/src/arch/l4 \
                  $(CONTRIB)/$(ARCH_DIR)/pci $(CONTRIB)/drivers/base/ \
                  $(CONTRIB)/lib $(PKGDIR_OBJ) $(CONTRIB)/net/core

##################################################################
# Sources for libdde_linux.a                                     #
##################################################################
SRC_DDE = cli_sti.c init_task.c init.c pci.c power.c process.c \
          res.c sched.c signal.c smp.c softirq.c timer.c \
          page_alloc.c kmem_cache.c kmalloc.c irq.c param.c \
		  vmalloc.c

# our implementation
SRC_C_libdde_linux26.a = $(addprefix arch/l4/, $(SRC_DDE))

ifeq ($(ARCH), x86)
SRC_C_libdde_linux26.a += $(ARCH_DIR)/lib/bitops.c
SRC_S_libdde_linux26_net.a += $(ARCH_DIR)/lib/checksum.S
SRC_S_libdde_linux26.a += $(ARCH_DIR)/lib/semaphore.S
endif

# + contrib stuff / slightly modified stuff
SRC_C_libdde_linux26.a += \
                          kernel/capability.c \
                          kernel/exit.c \
                          kernel/kthread.c \
                          kernel/mutex.c \
                          kernel/rcupdate.c \
                          kernel/resource.c \
                          kernel/rwsem.c \
                          kernel/sched.c \
                          kernel/spinlock.c \
                          kernel/sys.c \
                          kernel/wait.c \
                          kernel/workqueue.c \
                          lib/bitmap.c \
						  lib/bitrev.c \
                          lib/crc32.c \
                          lib/ctype.c \
                          lib/cpumask.c \
                          lib/iomap.c \
						  lib/hweight.c \
                          lib/klist.c \
                          lib/kobject.c \
                          lib/kref.c \
                          lib/rwsem.c \
                          lib/semaphore-sleepers.c \
                          lib/string.c \
                          lib/vsprintf.c \
                          mm/util.c \
                          drivers/base/attribute_container.c \
                          drivers/base/bus.c \
                          drivers/base/class.c \
                          drivers/base/core.c \
                          drivers/base/cpu.c \
                          drivers/base/dd.c \
                          drivers/base/driver.c \
                          drivers/base/init.c \
                          drivers/base/sys.c \
                          drivers/pci/access.c \
                          drivers/pci/bus.c \
                          drivers/pci/pci.c \
                          drivers/pci/pci-driver.c \
                          drivers/pci/probe.c \
                          drivers/pci/search.c \
                          drivers/pci/setup-bus.c \
                          drivers/pci/setup-res.c \
                          security/commoncap.c

##################################################################
# Sources for libdde_linux_net.a                                 #
##################################################################
SRC_C_libdde_linux26_net.a += \
                              arch/l4/net.c \
                              drivers/net/mii.c \
                              net/core/skbuff.c \
                              net/core/utils.c \
                              net/core/dev.c \
                              net/core/ethtool.c \
                              net/core/link_watch.c \
                              net/core/dev_mcast.c \
                              net/core/rtnetlink.c \
                              net/core/neighbour.c \
                              net/core/netevent.c \
                              net/ethernet/eth.c \
                              net/sched/sch_generic.c


##################################################################
# Sources for libdde_linux_block.a                               #
##################################################################
#
# (later...)
#SRC_C_libdde_linux26_block.a += \
#                              drivers/ide/ide.c

all::
lib/crc32.o : crc32table.h
lib/crc32.o : PRIVATE_INCDIR += .

crc32table.h : gen_crc32table
	@$(GEN_MESSAGE)
	$(VERBOSE)./$< >$@

gen_crc32table : lib/gen_crc32table.c
	@$(GEN_MESSAGE)
	$(VERBOSE)$(HOST_CC) -O2 -o $@ $<

include $(PKGDIR)/linux26/Makeconf

include $(L4DIR)/mk/lib.mk
