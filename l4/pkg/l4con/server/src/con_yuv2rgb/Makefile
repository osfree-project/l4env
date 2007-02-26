PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcon_yuv2rgb.a
SYSTEMS		= x86
NOTARGETSTOINSTALL = 1

MODE		= l4env
SRC_C		= cpu_accel.c yuv2rgb.c yuv2rgb_mmx.c

include $(L4DIR)/mk/lib.mk
