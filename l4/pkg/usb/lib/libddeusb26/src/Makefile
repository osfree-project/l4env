PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..
USB_CONTRIB_PATH ?= $(PKGDIR_ABS)/common/contrib
USB_MODIFIED_PATH ?= $(PKGDIR_ABS)/common

TARGET         = libddeusb26.a

SYSTEMS        = x86-l4v2


#include $(PKGDIR_OBJ)/Makeconf




VPATH =  $(USB_CONTRIB_PATH):$(USB_MODIFIED_PATH)



SRC_C		=        ddeusb_vhcd.c                 \
                         register.c                    \
				         drivers/usb/core/buffer.c      \
                         drivers/usb/core/driver.c      \
                         drivers/usb/core/file.c        \
                         drivers/usb/core/hcd.c         \
                         drivers/usb/core/hub.c         \
                         drivers/usb/core/notify.c      \
                         drivers/usb/core/urb.c         \
                         drivers/usb/core/config.c      \
                         drivers/usb/core/devio.c       \
                         drivers/usb/core/endpoint.c    \
                         drivers/usb/core/generic.c     \
                         drivers/usb/core/hcd-pci.c     \
                         drivers/usb/core/message.c     \
                         drivers/usb/core/sysfs.c       \
                         drivers/usb/core/usb.c         
			
			


PRIVATE_INCDIR	= $(PKGDIR_ABS)/common/contrib/drivers/usb/core \
          $(PKGDIR_ABS)/include \
          $(PKGDIR_ABS)/linux26/include \
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
