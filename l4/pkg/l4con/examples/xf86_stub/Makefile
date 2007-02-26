PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= pslim_drv.o
MODE		= host
LIBS		= -lcon.p -ldm_generic.p -ldm_mem.p -lnames.p \
		  -ll4util.p $(ROOTPLIB) -ll4sys.p
LIBCINCDIR	= $(addprefix -I,$(L4INCDIR) $(GCCINCDIR))
#LDFLAGS		= -r -nostdlib -nostartfiles
LDFLAGS		= -nostdlib -nostartfiles
LDSCRIPT	= $(call findfile,main_rel.ld,$(L4LIBDIR))

include $(L4DIR)/mk/Makeconf

ifneq ($(BUILD_LOADER),y)
# We must compile as PIC otherwise KIP syscalls don't work
$(error Please activate BUILD_LOADER in your l4/ config)
endif

CRT0		= $(CRTP_DEFAULT)

# Set to the Xorg/XFree86 xc directory. You have to invoke "make World" in
# that directory to make sure that the xc/exports/include directory exists.
XORGTREE	?=
XF86TREE	?=

WARNINGS_pslim.o = -Wall -Wpointer-arith -Wstrict-prototypes \
		  -Wmissing-prototypes -Wmissing-declarations \
		  -Wredundant-decls -Wnested-externs
CPPFLAGS_pslim.c = -Dlinux -D__i386__ \
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
CPPFLAGS_emul_l4env.c = $(CPPFLAGS_pslim.c)
CPPFLAGS_helper.c = -DXFree86LOADER -DIN_MODULE

# don't include Linux headers!
CPPFLAGS	= -nostdinc
CFLAGS		= -fPIC

ifneq ($(XORGTREE),)
# X.org tree defined in Makeconf.local
PRIVATE_INCDIR	= $(SRC_DIR) \
		  $(XORGTREE)/exports/include \
		  $(XORGTREE)/exports/include/X11 \
		  $(XORGTREE)/exports/include/X11/extensions \
		  $(XORGTREE)/include/fonts \
		  $(XORGTREE)/programs/Xserver/include \
		  $(XORGTREE)/programs/Xserver/hw/xfree86/common \
		  $(XORGTREE)/programs/Xserver/hw/xfree86/os-support \
		  $(XORGTREE)/programs/Xserver/hw/xfree86/xf24_32bpp \
		  $(XORGTREE)/programs/Xserver/hw/xfree86/xaa \
		  $(XORGTREE)/programs/Xserver/hw/xfree86/shadowfb \
		  $(XORGTREE)/programs/Xserver/hw/xfree86 \
		  $(XORGTREE)/programs/Xserver/fb \
		  $(XORGTREE)/programs/Xserver/mi \
		  $(XORGTREE)/programs/Xserver/afb \
		  $(XORGTREE)/programs/Xserver/mfb \
		  $(XORGTREE)/programs/Xserver/render \
		  $(XORGTREE)/programs/Xserver/miext/shadow
CPPFLAGS_pslim.c += -DL4CON_USE_DRIVER_FUNC
LDFLAGS		+= -shared
else
ifneq ($(XF86TREE),)
# XFree86 4.3.x tree defined in Makeconf.local
PRIVATE_INCDIR	= $(XF86TREE)/include \
		  $(XF86TREE)/include/X11 \
		  $(XF86TREE)/include/fonts \
		  $(XF86TREE)/include/extensions \
		  $(XF86TREE)/include/xserver \
		  $(XF86TREE)/programs/Xserver/hw/xfree86/common \
		  $(XF86TREE)/programs/Xserver/hw/xfree86/os-support \
		  $(XF86TREE)/programs/Xserver/hw/xfree86/os-support/bus \
		  $(XF86TREE)/programs/Xserver/hw/xfree86/xf24_32bpp \
		  $(XF86TREE)/programs/Xserver/hw/xfree86/xaa \
		  $(XF86TREE)/programs/Xserver/hw/xfree86/shadowfb \
		  $(XF86TREE)/programs/Xserver/hw/xfree86 \
		  $(XF86TREE)/programs/Xserver/include \
		  $(XF86TREE)/programs/Xserver/fb \
		  $(XF86TREE)/programs/Xserver/mi \
		  $(XF86TREE)/programs/Xserver/afb \
		  $(XF86TREE)/programs/Xserver/mfb \
		  $(XF86TREE)/programs/Xserver/render \
		  $(XF86TREE)/programs/Xserver/miext/shadow
CPPFLAGS_pslim.c += -DL4CON_USE_CFB24_32
LDFLAGS		+= -r
else
ifneq ($(BUILD_XORG),)
# Build with included headers for X.org 6.9.0
PRIVATE_INCDIR	= $(SRC_DIR) \
		  $(SRC_DIR)/include-xorg-6.9.0 \
		  $(SRC_DIR)/include-xorg-6.9.0/X11 \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/hw/xfree86 \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/hw/xfree86/common \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/hw/xfree86/os-support \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/hw/xfree86/os-support/bus \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/hw/xfree86/xaa \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/hw/xfree86/shadowfb \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/miext/shadow \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/fb \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/afb \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/mfb \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/mi \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/miext \
		  $(SRC_DIR)/include-xorg-6.9.0/xserver/render
CPPFLAGS_pslim.c += -DL4CON_USE_DRIVER_FUNC
LDFLAGS		+= -shared
else
# Build with included headers for XFree86 4.3.x
PRIVATE_INCDIR	= $(SRC_DIR) \
		  $(SRC_DIR)/include \
		  $(SRC_DIR)/include/X11 \
		  $(SRC_DIR)/include/fonts \
		  $(SRC_DIR)/include/xserver \
		  $(SRC_DIR)/include/xserver/mi \
		  $(SRC_DIR)/include/xserver/xf24_32bpp \
		  $(SRC_DIR)/include/xserver/fb \
		  $(SRC_DIR)/include/xserver/os-support \
		  $(SRC_DIR)/include/xserver/miext \
		  $(SRC_DIR)/include/xserver/render \
		  $(SRC_DIR)/include/xserver/xaa \
		  $(SRC_DIR)/include/xserver/shadowfb
CPPFLAGS_pslim.c += -DL4CON_USE_CFB24_32
LDFLAGS		+= -r
endif
endif
endif

# !!! Please don't link against -l4env-l4lx. Need special compiler flags !!!
# !!! for emul_l4env.c (mmap => xf86mmap).                               !!!
SRC_C		= pslim.c helper.c emul_l4env.c

INSTALLFILE_BIN_LOCAL = $(STRIP) --strip-unneeded $(1) \
			-o $(INSTALLDIR_LOCAL)/$(1)

include $(L4DIR)/mk/prog.mk
