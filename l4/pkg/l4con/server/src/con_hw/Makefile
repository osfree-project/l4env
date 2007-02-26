PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcon_hw.a
NOTARGETSTOINSTALL = 1

MODE		= l4env
SRC_C		= ati.c ati_vid.c \
		  matrox.c matrox_vid.c \
		  savage.c savage_vid.c \
		  vmware.c \
		  riva.c \
		  init.c pci.c iomem.c

include $(L4DIR)/mk/lib.mk
