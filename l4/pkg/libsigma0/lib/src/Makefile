PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libsigma0.a
MODE		= tiny
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2
SRC_C		= mem.c iomem.c anypage.c kip.c tbuf.c debug.c
SRC_S		= kip_area.S

include $(L4DIR)/mk/lib.mk
