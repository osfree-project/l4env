PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET_x86	= libcon_hw.a libcon_hw-ux.a
TARGET_arm	= libcon_hw.a
TARGET		= $(TARGET_$(ARCH))
NOTARGETSTOINSTALL = 1
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

MODE		= l4env
SRC_C_x86_libcon_hw.a = ati.c ati_vid.c \
		  ati128.c ati128_vid.c \
		  matrox.c matrox_vid.c \
		  savage.c savage_vid.c \
		  vmware.c \
		  riva.c \
		  intel.c \
		  init.c pci.c iomem.c
SRC_C_arm_libcon_hw.a = init.c iomem.c
SRC_C_libcon_hw.a = $(SRC_C_$(ARCH)_libcon_hw.a)

SRC_C_libcon_hw-ux.a = init-ux.c iomem.c

include $(L4DIR)/mk/lib.mk
