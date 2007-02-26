PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcon_yuv2rgb.a
SYSTEMS		= x86 arm
NOTARGETSTOINSTALL = 1

MODE		= l4env
SRC_C_x86	= cpu_accel.c yuv2rgb.c yuv2rgb_mmx.c
SRC_C_arm	= yuv2rgb.c
SRC_C		= $(SRC_C_$(ARCH))

include $(L4DIR)/mk/lib.mk
