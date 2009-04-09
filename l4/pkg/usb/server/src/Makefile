PKGDIR        ?= ../..
L4DIR         ?= $(PKGDIR)/../..
CONTRIB_PATH ?= $(PKGDIR_ABS)/contrib

SYSTEMS        = x86-l4v2

DEFAULT_RELOC  = 0x008e0000

-include $(PKGDIR_OBJ)/Makeconf

SERVERIDL = core.idl


TARGET         = usbcore

VPATH =  $(CONTRIB_PATH)

SRC_C		=main.c                        \
			 vudd.c                     \
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
                         driver/usb/core/usb.c         \
                         driver/usb/host/pci-quirks.c  \
			 driver/usb/host/ohci-hcd.c    \
                         driver/usb/host/uhci-hcd.c    \
			 driver/usb/host/ehci-hcd.c    \
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


PRIVATE_INCDIR	+= $(PKGDIR_ABS)/include \
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
