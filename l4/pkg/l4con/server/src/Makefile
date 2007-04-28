PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SUBDIRS		= con_hw con_yuv2rgb
SYSTEMS		= x86-l4v2 #arm-l4x0

include $(L4DIR)/mk/prog.mk

pre-obj:: $(SUBDIRS)
