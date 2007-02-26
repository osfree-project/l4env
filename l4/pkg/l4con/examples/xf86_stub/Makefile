PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= pslim_drv.o
MODE		= host
LIBS		= -lcon -ldm_generic -ldm_mem -lnames -ll4util -lrmgr
LIBCINCDIR	= $(addprefix -I,$(L4INCDIR) $(GCCINCDIR))
LDFLAGS		= -r -nostdlib -nostartfiles
LDSCRIPT	= $(call findfile,main_rel.ld,$(L4LIBDIR))

XF86TREE	= ..

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
CPPFLAGS_emul_l4rm.c = $(CPPFLAGS_pslim.c)
CPPFLAGS_helper.c = -DXFree86LOADER -DIN_MODULE

# don't include Linux headers!
CPPFLAGS	= -nostdinc

ifeq ($(XF86TREE),..)
PRIVATE_INCDIR	= ../include \
		  ../include/X11 \
		  ../include/fonts \
		  ../include/xserver \
		  ../include/xserver/mi \
		  ../include/xserver/xf24_32bpp \
		  ../include/xserver/fb \
		  ../include/xserver/os-support \
		  ../include/xserver/miext \
		  ../include/xserver/render \
		  ../include/xserver/xaa
else
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
		  $(XF86TREE)/programs/Xserver/hw/xfree86 \
		  $(XF86TREE)/programs/Xserver/include \
		  $(XF86TREE)/programs/Xserver/fb \
		  $(XF86TREE)/programs/Xserver/mi \
		  $(XF86TREE)/programs/Xserver/afb \
		  $(XF86TREE)/programs/Xserver/mfb \
		  $(XF86TREE)/programs/Xserver/render \
		  $(XF86TREE)/programs/Xserver/miext/shadow
endif

SRC_C		= pslim.c helper.c emul_l4rm.c

INSTALLFILE_BIN_LOCAL = $(STRIP) --strip-unneeded $(1) \
			-o $(INSTALLDIR_LOCAL)/$(1)

include $(L4DIR)/mk/prog.mk
