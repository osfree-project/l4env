SYSTEMS         := x86-l4v2 amd64-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = vscrtest
LIBS             = -ll4dope -lvscreen -ldata
DEFAULT_RELOC    = 0x00800000

vpath % $(PKGDIR_ABS)/examples/vscrtest/common

PRIVATE_INCDIR   = $(PKGDIR_ABS)/examples/vscrtest/include \
                   $(PKGDIR_ABS)/include \
                   $(PKGDIR_ABS)/server/include

PRIVATE_LIBDIR   = $(PKGDIR_OBJ)/examples/vscrtest/data/OBJ-$(SYSTEM) \
                   /usr/lib

SRC_C            = vscrtest.c voxel.c thread.c startup.c \
                   bump.c feedback.c fountain.c 

include $(L4DIR)/mk/prog.mk

ifeq ($(USE_OSKIT),y)
LIBS		+= -loskit_freebsd_m
endif
