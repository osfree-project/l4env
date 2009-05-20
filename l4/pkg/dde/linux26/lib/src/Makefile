PKGDIR     ?= ../../..
L4DIR      ?= $(PKGDIR)/../..
CONTRIB    ?= $(PKGDIR)/linux26/contrib

-include $(PKGDIR_OBJ)/Makeconf

ifeq ($(CONFIG_DDE26_COMMON),y)
TARGET += libdde_linux26.o.a
endif

ifeq ($(CONFIG_DDE26_NET),y)
TARGET += libdde_linux26_net.a
endif

ifeq ($(CONFIG_DDE26_BLOCK),y)
TARGET += libdde_linux26_block.a
endif

ifeq ($(CONFIG_DDE26_SOUND),y)
TARGET += libdde_linux26_sound.a
endif

ifeq ($(CONFIG_DDE26_CHAR),y)
TARGET += libdde_linux26_char.a
endif

SYSTEMS = x86-l4v2

ifeq ($(ARCH), x86)
ARCH_DIR = arch/x86
endif

ifeq ($(ARCH), arm)
ARCH_DIR = arch/arm
MARCH = realview
DEFINES += -D__LINUX_ARM_ARCH__=6
endif

# contrib sources are in $(CONTRIB)
vpath %.c     $(CONTRIB)
vpath %.S    $(CONTRIB)

PRIVATE_INCDIR += $(CONTRIB)/drivers/pci $(PKGDIR)/linux26/lib/src/arch/l4 \
                  $(CONTRIB)/$(ARCH_DIR)/pci $(CONTRIB)/drivers/base/ \
                  $(CONTRIB)/lib $(PKGDIR_OBJ) $(CONTRIB)/net/core $(CONTRIB)/fs

##################################################################
# Sources for libdde_linux.a                                     #
##################################################################
SRC_DDE = cli_sti.c fs.c hw-helpers.c init_task.c init.c pci.c power.c \
		  process.c res.c sched.c signal.c smp.c softirq.c timer.c \
          page_alloc.c kmem_cache.c kmalloc.c irq.c param.c \
		  vmalloc.c vmstat.c mm-helper.c

# our implementation
SRC_C_libdde_linux26.o.a = $(addprefix arch/l4/, $(SRC_DDE))

ifeq ($(ARCH), x86)
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/semaphore_32.S
SRC_C_libdde_linux26.o.a += lib/rwsem.c
SRC_C_libdde_linux26.o.a += $(ARCH_DIR)/kernel/pci-dma.c
SRC_C_libdde_linux26.o.a += $(ARCH_DIR)/kernel/pci-nommu.c
SRC_S_libdde_linux26_net.a += $(ARCH_DIR)/lib/checksum_32.S
endif

ifeq ($(ARCH), arm)
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/changebit.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/clearbit.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/div64.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/findbit.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/memzero.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/setbit.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/testclearbit.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/testchangebit.S
SRC_S_libdde_linux26.o.a += $(ARCH_DIR)/lib/testsetbit.S
SRC_C_libdde_linux26.o.a += $(ARCH_DIR)/kernel/semaphore.c
SRC_C_libdde_linux26.o.a += $(ARCH_DIR)/kernel/traps.c
SRC_C_libdde_linux26.o.a += $(ARCH_DIR)/mach-$(MARCH)/clock.c
SRC_C_libdde_linux26.o.a += $(ARCH_DIR)/mach-$(MARCH)/realview_eb.c
SRC_C_libdde_linux26.o.a += lib/rwsem-spinlock.c
SRC_C_libdde_linux26.o.a += drivers/amba/bus.c
endif

# + contrib stuff / slightly modified stuff
SRC_C_libdde_linux26.o.a += \
                          kernel/exit.c \
                          kernel/kthread.c \
                          kernel/mutex.c \
						  kernel/notifier.c \
                          kernel/resource.c \
                          kernel/rwsem.c \
                          kernel/sched.c \
						  kernel/semaphore.c \
                          kernel/sys.c \
						 kernel/time.c \
						  kernel/timer.c \
                          kernel/wait.c \
                          kernel/workqueue.c \
                          lib/bitmap.c \
						  lib/bitrev.c \
                          lib/crc32.c \
                          lib/ctype.c \
                          lib/cpumask.c \
						  lib/find_next_bit.c \
						  lib/hexdump.c \
						  lib/idr.c \
                          lib/iomap.c \
						  lib/hweight.c \
						  lib/kasprintf.c \
						  lib/kernel_lock.c \
                          lib/klist.c \
                          lib/kobject.c \
                          lib/kref.c \
						  lib/parser.c \
						  lib/proportions.c \
						  lib/radix-tree.c \
						  lib/scatterlist.c \
						  lib/sha1.c \
                          lib/string.c \
                          lib/vsprintf.c \
						  mm/mempool.c \
						  mm/swap.c \
                          mm/util.c \
                          drivers/base/attribute_container.c \
                          drivers/base/bus.c \
                          drivers/base/class.c \
                          drivers/base/core.c \
                          drivers/base/cpu.c \
                          drivers/base/dd.c \
						  drivers/base/devres.c \
                          drivers/base/driver.c \
                          drivers/base/init.c \
						  drivers/base/map.c \
						  drivers/base/platform.c \
                          drivers/base/sys.c \
                          drivers/pci/access.c \
                          drivers/pci/bus.c \
						  drivers/pci/hotplug-pci.c \
                          drivers/pci/pci.c \
                          drivers/pci/pci-driver.c \
                          drivers/pci/probe.c \
                          drivers/pci/search.c \
                          drivers/pci/setup-bus.c \
                          drivers/pci/setup-res.c

##################################################################
# Sources for libdde_linux_net.a                                 #
##################################################################
SRC_C_libdde_linux26_net.a += \
                              arch/l4/net.c \
                              drivers/net/mii.c \
                              net/core/dev.c \
                              net/core/dev_mcast.c \
                              net/core/ethtool.c \
                              net/core/link_watch.c \
                              net/core/neighbour.c \
                              net/core/netevent.c \
							  net/core/net-sysfs.c \
							  net/core/net_namespace.c \
                              net/core/rtnetlink.c \
                              net/core/skbuff.c \
							  net/core/skb_dma_map.c \
                              net/core/utils.c \
                              net/ethernet/eth.c \
                              net/sched/sch_generic.c

##################################################################
# Sources for libdde_linux_sound.a                               #
##################################################################
SRC_C_libdde_linux26_sound.a += \
								sound/sound_core.c \
								arch/l4/sound.c

##################################################################
# Sources for libdde_linux_block.a                               #
##################################################################
#
SRC_C_libdde_linux26_block.a += \
								arch/l4/inodes.c      \
								block/blk-barrier.c   \
								block/blk-core.c      \
								block/blk-exec.c      \
								block/blk-ioc.c       \
								block/blk-merge.c      \
								block/blk-settings.c      \
								block/blk-softirq.c \
								block/blk-sysfs.c      \
								block/blk-tag.c      \
								block/blk-timeout.c      \
								block/elevator.c      \
								block/genhd.c         \
								block/noop-iosched.c  \
								block/ioctl.c         \
								block/scsi_ioctl.c    \
								mm/backing-dev.c      \
								mm/bounce.c           \
								mm/page-writeback.c   \
								fs/bio.c              \
								fs/block_dev.c        \
								fs/buffer.c           \
								fs/filesystems.c
##################################################################
# Sources for libdde_linux_char.a                               #
##################################################################
SRC_C_libdde_linux26_char.a += \
								arch/l4/inodes.c      \
								fs/char_dev.c

all::
lib/crc32.o : crc32table.h
lib/crc32.o : PRIVATE_INCDIR += .
kernel/time.o : timeconst.h
kernel/time.o : PRIVATE_INCDIR += .

timeconst.h : $(SRC_DIR)/kernel/timeconst.pl
	@$(GEN_MESSAGE)
	$(VERBOSE)$< 250 >$@

crc32table.h : gen_crc32table
	@$(GEN_MESSAGE)
	$(VERBOSE)./$< >$@

gen_crc32table : lib/gen_crc32table.c
	@$(GEN_MESSAGE)
	$(VERBOSE)$(HOST_CC) -O2 -o $@ $<

include $(PKGDIR)/linux26/Makeconf

include $(L4DIR)/mk/lib.mk
