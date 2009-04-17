PKGDIR        ?= ../../..
L4DIR         ?= $(PKGDIR)/../..
USB_CONTRIB_PATH ?= $(PKGDIR_ABS)/contrib
MY_CONTRIB_PATH ?= $(PKGDIR_ABS)/examples/usbcam/contrib

TARGET         = usbcam

SYSTEMS        = x86-l4v2

DEFAULT_RELOC  = 0x03010000

#include $(PKGDIR_OBJ)/Makeconf




VPATH =  $(USB_CONTRIB_PATH):$(MY_CONTRIB_PATH)


QUICKCAM_SRC_C =    linux/drivers/media/video/usbvideo/usbvideo.c \
                    linux/drivers/media/video/usbvideo/quickcam_messenger.c


UVCVIDEO_SRC_C =    linux/drivers/media/video/uvc/uvc_driver.c\
                    linux/drivers/media/video/uvc/uvc_queue.c\
                    linux/drivers/media/video/uvc/uvc_v4l2.c\
                    linux/drivers/media/video/uvc/uvc_video.c\
                    linux/drivers/media/video/uvc/uvc_ctrl.c \
                    linux/drivers/media/video/uvc/uvc_status.c\
		            linux/drivers/media/video/uvc/uvc_isight.c


GSPCA_SRC_C =       linux/drivers/media/video/gspca/gspca.c \
					linux/drivers/media/video/gspca/spca500.c\
					linux/drivers/media/video/gspca/spca501.c\
					linux/drivers/media/video/gspca/spca505.c\
					linux/drivers/media/video/gspca/spca506.c\
					linux/drivers/media/video/gspca/spca508.c\
					linux/drivers/media/video/gspca/spca561.c\
					linux/drivers/media/video/gspca/zc3xx.c\


LIBV4LCONV_SRC_C = libv4lconvert/bayer.c\
libv4lconvert/jidctflt.c\
libv4lconvert/pac207.c\
libv4lconvert/spca501.c\
libv4lconvert/crop.c\
libv4lconvert/libv4lconvert.c\
libv4lconvert/rgbyuv.c\
libv4lconvert/spca561-decompress.c\
libv4lconvert/flip.c\
libv4lconvert/sn9c10x.c\
libv4lconvert/sq905c.c\
libv4lconvert/hm12.c\
libv4lconvert/mr97310a.c\
libv4lconvert/sn9c20x.c\
libv4lconvert/tinyjpeg.c

CPIA_SRC_C		=   \
					linux/drivers/media/video/cpia.c \
                    linux/drivers/media/video/cpia_usb.c \

V4L_SRC_C      =    linux/drivers/media/video/v4l2-common.c \
					linux/drivers/media/video/v4l1-compat.c \
					linux/drivers/media/video/v4l2-device.c \
					linux/drivers/media/video/v4l2-ioctl.c

					#drivers/media/video/compat_ioctl32.c \
		
SRC_C           =   main.c \
                    support.c \
                    usbcam.c \
					$(LIBV4LCONV_SRC_C) \
					$(V4L_SRC_C)\
					$(CPIA_SRC_C) \
					$(GSPCA_SRC_C) \
					$(QUICKCAM_SRC_C) \
					$(UVCVIDEO_SRC_C)

LIBS			+= -ll4dope          \
                   -lvscreen         \
				   --whole-archive -lddeusb26  --no-whole-archive   \
				   -lddeusb          \
				   -lusb_core_client \
				   -ldde_linux26     \
				   -lddekit          \
				   -lio		         \
				   -lomega0 	     \
				   -llist_alloc	 

PRIVATE_INCDIR	= \
		$(MY_CONTRIB_PATH)/v4l\
		  $(MY_CONTRIB_PATH)/linux/include \
		$(PKGDIR_ABS)/linux26/include \
		  $(MY_DDE_INCDIR)              \
		  $(MY_LINUX26_INCDIR)          \
		  $(OBJ_BASE)/include/uclibc    \
		  $(PKGDIR_ABS)/server/include \





LIBCINCDIR  = -nostdinc $(I_GCCINCDIR)
DEFINES     = -DLINUX_VERSION_CODE=849221 -DKERN_CONT=""  -D"KERNEL_VERSION(a,b,c)=(((a) << 16) + ((b) << 8) + (c))"  -DDDEUSB26_GADGET -DCONFIG_USB_GSPCA_MODULE=1 -DVID_HARDWARE_GSPCA=0xFF   -DDEBUG -D__KERNEL__ -DDDE_LINUX -DGSPCA_VERSION=\"01.00.20\"  -DCONFIG_VIDEO_V4L1_COMPAT  $(KBUILD_DEFINES)


#DEFINES += -DGSPCA_ENABLE_DEBUG

CPPFLAGS 	+= $(KBUILD_CPPFLAGS)

include $(PKGDIR)/../dde/linux26/Makeconf

include $(L4DIR)/mk/prog.mk

foo :
	@echo $(L4INCDIR)
	@echo $(OBJ_BASE)
