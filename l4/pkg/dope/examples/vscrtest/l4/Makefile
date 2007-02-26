SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..

TARGET           = vscrtest
LIBS             = -ll4dope -lvscreen -lm -ldata
DEFAULT_RELOC    = 0x00880000

vpath % $(PKGDIR)/examples/vscrtest/common

PRIVATE_INCDIR   = $(PKGDIR)/examples/vscrtest/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/server/include

PRIVATE_LIBDIR   = $(PKGDIR)/examples/vscrtest/data/OBJ-$(SYSTEM) \
                   /usr/lib

SRC_C            = vscrtest.c voxel.c thread.c startup.c \
                   bump.c feedback.c fountain.c 

include $(L4DIR)/mk/prog.mk
