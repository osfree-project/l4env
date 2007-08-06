PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SUBDIRS		= con_hw con_yuv2rgb
SYSTEMS		= x86-l4v2 amd64-l4v2 arm-l4v2

include $(L4DIR)/mk/prog.mk

pre-obj:: $(SUBDIRS)
