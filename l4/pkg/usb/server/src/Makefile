PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..
CONTRIB_PATH ?= $(PKGDIR_ABS)/common/contrib
MODIFIED_PATH ?= $(PKGDIR_ABS)/common

SYSTEMS        = x86-l4v2

DEFAULT_RELOC  = 0x008e0000

-include $(PKGDIR_OBJ)/Makeconf

SERVERIDL = core.idl


TARGET         = usbcore

VPATH =  $(CONTRIB_PATH):$(MODIFIED_PATH)

SRC_C		=main.c                        \
			 vudd.c                     \
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
                         drivers/usb/core/usb.c         \
                         drivers/usb/host/pci-quirks.c  \
			 drivers/usb/host/ohci-hcd.c    \
                         drivers/usb/host/uhci-hcd.c    \
			 drivers/usb/host/ehci-hcd.c    \
#			 driver/usb/host/u132-hcd.c    \
#			 driver/usb/host/sl811-hcd.c   \
#			 driver/usb/host/isp116x-hcd.c 
			
			

CFLAGS += -DUSB_CORE -Wall

LIBS			+= \
                           -ldde_linux26     \
-lddekit          \
			   -lio		     \
			   -lomega0 	     \
			   -lusb_gadget_client \
			   -llist_alloc	 


PRIVATE_INCDIR	+= $(CONTRIB_PATH)/drivers/usb/core \
					$(PKGDIR_ABS)/include \
		           $(MY_DDE_INCDIR)              \
			       $(MY_LINUX26_INCDIR)   \
		           $(OBJ_BASE)/include/uclibc    \
		           $(PKGDIR_ABS)/server/include


LIBCINCDIR  = -nostdinc $(I_GCCINCDIR)

DEFINES    += -D__KERNEL__ -DDDE_LINUX $(KBUILD_DEFINES)

CPPFLAGS   += $(KBUILD_CPPFLAGS)

include $(PKGDIR)/../dde/linux26/Makeconf

include $(L4DIR)/mk/prog.mk

foo :
	@echo $(L4INCDIR)
	@echo $(OBJ_BASE)
