SYSTEMS         := x86-l4v2 amd64-l4v2
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = vscrtest
LIBS             = -ll4dope -lvscreen -ldata
DEFAULT_RELOC    = 0x00800000

vpath % $(SRC_DIR)/../common

PRIVATE_INCDIR   = $(SRC_DIR)/../include \
                   $(PKGDIR_ABS)/include \
                   $(PKGDIR_ABS)/server/include

PRIVATE_LIBDIR   = $(OBJ_DIR)/../data/OBJ-$(SYSTEM) \
                   /usr/lib

SRC_C            = vscrtest.c voxel.c thread.c startup.c \
                   bump.c feedback.c fountain.c 

include $(L4DIR)/mk/prog.mk

ifeq ($(USE_OSKIT),y)
LIBS		+= -loskit_freebsd_m
endif
