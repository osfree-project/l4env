PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..
USB_CONTRIB_PATH ?= $(PKGDIR_ABS)/contrib

TARGET         = libddeusb26.a

SYSTEMS        = x86-l4v2


#include $(PKGDIR_OBJ)/Makeconf




VPATH =  $(USB_CONTRIB_PATH)



SRC_C		=        ddeusb_vhcd.c                 \
                         register.c                    \
				         driver/usb/core/buffer.c      \
                         driver/usb/core/devices.c     \
                         driver/usb/core/driver.c      \
                         driver/usb/core/file.c        \
                         driver/usb/core/hcd.c         \
                         driver/usb/core/hub.c         \
                         driver/usb/core/notify.c      \
                         driver/usb/core/urb.c         \
                         driver/usb/core/config.c      \
                         driver/usb/core/devio.c       \
                         driver/usb/core/endpoint.c    \
                         driver/usb/core/generic.c     \
                         driver/usb/core/hcd-pci.c     \
                         driver/usb/core/inode.c       \
                         driver/usb/core/message.c     \
                         driver/usb/core/sysfs.c       \
                         driver/usb/core/usb.c         
			
			


PRIVATE_INCDIR	= $(PKGDIR_ABS)/linux26/include \
		  $(MY_DDE_INCDIR)              \
		  $(MY_LINUX26_INCDIR)          \
		  $(OBJ_BASE)/include/uclibc    \
		  $(PKGDIR_ABS)/server/include \





DEFINES     =  -D__KERNEL__ -DDDE_LINUX   $(KBUILD_DEFINES)

include $(PKGDIR)/../dde/linux26/Makeconf

include $(L4DIR)/mk/lib.mk

foo :
	@echo $(L4INCDIR)
	@echo $(OBJ_BASE)
