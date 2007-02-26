PKGDIR		?= ..
L4DIR		?= $(PKGDIR)/../..

TARGET		= l4ide
MODE		= l4env
DEFAULT_RELOC	= 0x0900000
SYSTEMS		= x86-l4v2

SRC_C		= pci/generic.c pci/via82cxxx.c pci/piix.c \
		  pci/aec62xx.c pci/alim15x3.c pci/amd74xx.c pci/trm290.c \
		  pci/cmd640.c pci/cmd64x.c pci/cs5530.c pci/cy82c693.c pci/hpt34x.c pci/hpt366.c \
		  pci/ns87415.c pci/pdc202xx_new.c pci/pdc202xx_old.c pci/rz1000.c pci/sc1200.c \
		  pci/serverworks.c pci/siimage.c pci/sis5513.c pci/slc90e66.c pci/triflex.c \
		  partitions/check.c partitions/msdos.c partitions/acorn.c partitions/amiga.c \
		  partitions/ldm.c partitions/mac.c partitions/nec98.c partitions/osf.c \
		  partitions/sgi.c partitions/sun.c partitions/ultrix.c partitions/atari.c \
		  block/elevator.c block/ll_rw_blk.c block/noop-iosched.c block/bio.c \
		  block/dummies.c block/genhd.c block/base.c block/block_dev.c block/cdrom.c \
		  interface/driver.c interface/command.c interface/notification.c \
	          ide-io.c ide-probe.c ide-iops.c ide-taskfile.c ide.c ide-lib.c ide-default.c \
		  ide-disk.c ide-cd.c ide-floppy.c ide-generic.c \
		  setup-pci.c \
		  ide-dma.c \
		  main.c

### specialties
PRIVATE_INCDIR	= $(PKGDIR)/server $(PKGDIR)/server/include $(PKGDIR)/include \
                  $(DDE26_INCDIR) $(LINUX26_INCDIR)

LIBS		= -ldde_linux26 -lio -lomega0 -ll4env -lgeneric_blk-server \
		  -llist_alloc

DEFINES = -D__KERNEL__ -DDDE_LINUX -DDDE_IDE
WARNINGS = -Wall -Wstrict-prototypes

### act the role of ...
include $(L4DIR)/mk/prog.mk

ifdef DEBUG
  DEFINES += -DDEBUG_ERRORS -DDEBUG_ASSERTIONS
endif

