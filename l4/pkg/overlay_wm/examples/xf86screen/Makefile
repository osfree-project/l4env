PKGDIR          ?= ../..
L4DIR           ?= $(PKGDIR)/../..

ifeq ($(XF86TREE),)
ifeq ($(XORGTREE),)
all::
	$(error *** you must specify XF86TREE=/where/your/xfree/sources/reside/xc or XORGTREE ***)
endif
endif

TARGET          = ovlscreen_drv.o
SRC_C           = ovlscreen.c
MODE            = host
LIBS		= -ll4ovlscreen.p -ldm_generic.p -ldm_mem.p -lnames.p \
		  -ll4util.p $(ROOTPLIB) -ll4sys.p -ll4sys.p
LIBCINCDIR      = $(addprefix -I,$(L4INCDIR))
LDFLAGS         = -shared -nostdlib -nostartfiles
LDSCRIPT        = $(call findfile,main_rel.ld,$(L4LIBDIR))
CRT0            = $(CRTP_DEFAULT)

CFLAGS          = -fPIC
CPPFLAGS        = -fPIC

WARNINGS_ovlscreen.o = -Wall -Wpointer-arith -Wstrict-prototypes \
                  	-Wmissing-prototypes -Wmissing-declarations \
                  	-Wredundant-decls -Wnested-externs

CPPFLAGS_ovlscreen.c = \
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

PRIVATE_INCDIR	 = $(SRC_DIR)
ifneq ($(XORGTREE),)
PRIVATE_INCDIR += .. \
                 $(XORGTREE)/include \
                 $(XORGTREE)/include/X11 \
                 $(XORGTREE)/include/fonts \
		 $(XORGTREE)/include/extensions \
                 $(XORGTREE)/include/xserver \
                 $(XORGTREE)/programs/Xserver/hw/xfree86/common \
                 $(XORGTREE)/programs/Xserver/hw/xfree86/os-support \
                 $(XORGTREE)/programs/Xserver/hw/xfree86/os-support/bus \
                 $(XORGTREE)/programs/Xserver/hw/xfree86/xf24_32bpp \
                 $(XORGTREE)/programs/Xserver/hw/xfree86/xaa \
                 $(XORGTREE)/programs/Xserver/hw/xfree86/shadowfb \
                 $(XORGTREE)/programs/Xserver/hw/xfree86 \
                 $(XORGTREE)/programs/Xserver/include \
                 $(XORGTREE)/programs/Xserver/fb \
                 $(XORGTREE)/programs/Xserver/mi \
                 $(XORGTREE)/programs/Xserver/afb \
                 $(XORGTREE)/programs/Xserver/mfb \
                 $(XORGTREE)/programs/Xserver/render \
                 $(XORGTREE)/programs/Xserver/miext/shadow \
                 $(PKGDIR)/include
else
PRIVATE_INCDIR += $(XF86TREE)/include \
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
                 $(PKGDIR)/include
endif

INSTALLFILE_BIN_LOCAL = $(STRIP) --strip-unneeded $(1) \
                        -o $(INSTALLDIR_LOCAL)/$(1)

include $(L4DIR)/mk/prog.mk

ifeq ($(FORCE_GCC295),yes)
include $(PKGDIR)/examples/xf86input/Makeconf
endif
