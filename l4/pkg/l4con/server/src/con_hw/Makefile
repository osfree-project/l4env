PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET_x86	= libcon_hw.a libcon_hw-ux.a
TARGET_arm	= libcon_hw.a
TARGET		= $(TARGET_$(ARCH))
NOTARGETSTOINSTALL = 1
SYSTEMS		= x86-l4v2 arm-l4x0

MODE		= l4env
OPTS		= -g -Os $(CARCHFLAGS_$(ARCH)) $(CARCHFLAGS_$(ARCH)_$(CPU))
SRC_C_x86_libcon_hw.a = ati.c ati_vid.c \
		  ati128.c ati128_vid.c \
		  radeon.c \
		  matrox.c matrox_vid.c \
		  savage.c savage_vid.c \
		  vmware.c \
		  riva.c \
		  intel.c \
		  init.c pci.c iomem.c
SRC_C_arm_libcon_hw.a = iomem.c
SRC_C_libcon_hw.a = $(SRC_C_$(ARCH)_libcon_hw.a)

SRC_C_libcon_hw-ux.a = init-ux.c iomem.c ux.c

include $(L4DIR)/mk/lib.mk
