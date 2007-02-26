PKGDIR          ?= ../../..
L4DIR           ?= $(PKGDIR)/../..
TARGET           = vscrtest
SYSTEMS          = x86-linux
MODE             = host
LIBS             = -llinuxdope -lpthread -lm -lvscreen -ldata
SRC_C            = vscrtest.c startup.c thread.c \
                   voxel.c bump.c fountain.c feedback.c
PRIVATE_INCDIR   = . \
                   $(PKGDIR)/examples/vscrtest/include \
                   $(PKGDIR)/include \
                   $(PKGDIR)/server/include
PRIVATE_LIBDIR   = $(PKGDIR)/lib/dope/linux/OBJ-$(SYSTEM) \
                   $(PKGDIR)/lib/vscreen/linux/OBJ-$(SYSTEM) \
                   $(PKGDIR)/examples/vscrtest/data/OBJ-$(SYSTEM)

vpath % $(PKGDIR)/examples/vscrtest/common 

include $(L4DIR)/mk/prog.mk
