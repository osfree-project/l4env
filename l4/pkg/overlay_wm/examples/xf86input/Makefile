PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

ifeq ($(XF86TREE),)
all::
	$(warning *** you should specify XF86TREE=/where/your/xfree/sources/reside/xc ***)
endif

# sorry for my ignorance #
XF86TREE        ?= /home/nf2/src/xfree-4.2.1

TARGET           = lxevent_drv.o
SRC_C            = lxevent.c
MODE             = host
LDFLAGS          = -r -nostdlib -nostartfiles
LDSCRIPT         = $(call findfile,main_rel.ld,$(L4LIBDIR))

CPPFLAGS_lxevent.c = \
                 -Dlinux -D__i386__ \
                 -D_POSIX_C_SOURCE=199309L -D_POSIX_SOURCE \
                 -D_XOPEN_SOURCE -D_BSD_SOURCE -D_SVID_SOURCE -D_GNU_SOURCE \
                 -DSHAPE -DXINPUT -DXKB -DLBX -DXAPPGROUP -DXCSECURITY \
                 -DTOGCUP -DXF86BIGFONT -DDPMSExtension \
                 -DPIXPRIV -DPANORAMIX -DRENDER -DGCCUSESGAS \
                 -DAVOID_GLYPHBLT -DPIXPRIV -DSINGLEDEPTH -DXFreeXDGA \
                 -DXvExtension -DXFree86LOADER -DXFree86Server \
                 -DXF86VIDMODE -DSMART_SCHEDULE -DBUILDDEBUG \
                 -DX_BYTE_ORDER=X_LITTLE_ENDIAN -DNDEBUG \
                 -DFUNCPROTO=15 -DNARROWPROTO -DIN_MODULE -DXFree86Module

PRIVATE_INCDIR = $(XF86TREE)/include \
                 $(XF86TREE)/include/X11 \
                 $(XF86TREE)/include/fonts \
                 $(XF86TREE)/include/extensions \
                 $(XF86TREE)/include/xserver \
                 $(XF86TREE)/programs/Xserver/hw/xfree86/common \
                 $(XF86TREE)/programs/Xserver/hw/xfree86/os-support \
                 $(XF86TREE)/programs/Xserver/hw/xfree86/os-support/bus \
                 $(XF86TREE)/programs/Xserver/hw/xfree86/xf24_32bpp \
                 $(XF86TREE)/programs/Xserver/hw/xfree86/xaa \
                 $(XF86TREE)/programs/Xserver/hw/xfree86 \
                 $(XF86TREE)/programs/Xserver/include \
                 $(XF86TREE)/programs/Xserver/fb \
                 $(XF86TREE)/programs/Xserver/mi \
                 $(XF86TREE)/programs/Xserver/afb \
                 $(XF86TREE)/programs/Xserver/mfb \
                 $(XF86TREE)/programs/Xserver/render \
                 $(XF86TREE)/programs/Xserver/miext/shadow \
                 $(PKGDIR)/include \
                 .

INSTALLFILE_BIN_LOCAL = $(STRIP) --strip-unneeded $(1) \
                        -o $(INSTALLDIR_LOCAL)/$(1)

include $(L4DIR)/mk/prog.mk

ifeq ($(FORCE_GCC295),yes)
include $(PKGDIR)/examples/xf86input/Makeconf
endif

ifneq ($(SYSTEM),)
all::
	objcopy -G lxeventModuleData $(TARGET) $(TARGET)
endif
