PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..
USB_CONTRIB_PATH ?= $(PKGDIR_ABS)/contrib
MY_CONTRIB_PATH ?= $(PKGDIR_ABS)/examples/usbhid/src/contrib

TARGET         = usbhid

SYSTEMS        = x86-l4v2

DEFAULT_RELOC  = 0x00cc0000

#include $(PKGDIR_OBJ)/Makeconf




VPATH =  $(USB_CONTRIB_PATH):$(MY_CONTRIB_PATH)



SRC_C		=        main.c                    \
			         l4evdev.c                     \
                     driver/usb/input/hid-core.c   \
                     dummy.c                       \
		             driver/hid/hid-core.c         \
		             driver/hid/hid-input.c       \
		             driver/input/input.c
			
			

LIBS			+= --whole-archive -lddeusb26  --no-whole-archive  \
			   -lddeusb          \
			   -lusb_core_client \
			   -ldde_linux26     \
			   -lddekit          \
			   -lio		     \
			   -lomega0 	     \
			   -llist_alloc	 

PRIVATE_INCDIR	= $(PKGDIR_ABS)/include \
		  $(MY_DDE_INCDIR)              \
		  $(MY_LINUX26_INCDIR)          \
		  $(OBJ_BASE)/include/uclibc    \
		  $(PKGDIR_ABS)/server/include \





LIBCINCDIR  = -nostdinc $(I_GCCINCDIR)
DEFINES     = -DDDEUSB26_GADGET -D__KERNEL__ -DDDE_LINUX   $(KBUILD_DEFINES)
CPPFLAGS 	+= $(KBUILD_CPPFLAGS)

include $(PKGDIR)/../dde/linux26/Makeconf

include $(L4DIR)/mk/prog.mk

foo :
	@echo $(L4INCDIR)
	@echo $(OBJ_BASE)
