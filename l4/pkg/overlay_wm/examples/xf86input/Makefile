PKGDIR     ?= ../..
L4DIR      ?= $(PKGDIR)/../..
CONDRV_DIR  = $(L4DIR)/pkg/l4con/examples/xf86_stub
X11_INCDIR ?= /usr/include/xorg

ifeq ($(wildcard $(X11_INCDIR)),)
$(warning Please define X11_INCDIR. This variable should point to the Xorg development headers)
$(warning On Debian, you may just install the xserver-xorg-dev package.)
TARGET     =
else
TARGET     = lxevent_drv.so
endif

SRC_C      = lxevent.c
MODE       = linux
LDFLAGS    = -nostdlib -nostartfiles -shared
LDSCRIPT   = $(call findfile,main_rel.ld,$(L4LIBDIR))
CRT0       = $(CRTP_DEFAULT)
CFLAGS     = -fPIC

WARNINGS   = -Wall -Wpointer-arith -Wstrict-prototypes \
             -Wmissing-prototypes -Wmissing-declarations \
             -Wredundant-decls -Wnested-externs

DEFINES    = -Dlinux -D__i386__ -D_POSIX_C_SOURCE=199309L \
             -D_POSIX_SOURCE -D_XOPEN_SOURCE -D_BSD_SOURCE \
             -D_SVID_SOURCE -D_GNU_SOURCE -DSHAPE -DXINPUT \
             -DXKB -DLBX -DXAPPGROUP -DXCSECURITY -DTOGCUP \
             -DXF86BIGFONT -DDPMSExtension -DPIXPRIV \
             -DPANORAMIX -DRENDER -DGCCUSESGAS \
             -DAVOID_GLYPHBLT -DPIXPRIV -DSINGLEDEPTH \
             -DXFreeXDGA -DXvExtension -DXFree86LOADER \
             -DXFree86Server -DXF86VIDMODE -DSMART_SCHEDULE \
             -DBUILDDEBUG -DX_BYTE_ORDER=X_LITTLE_ENDIAN \
             -DNDEBUG -DFUNCPROTO=15 -DNARROWPROTO -DIN_MODULE \
             -DXFree86Module

DEFINES   += -DXORG71

INSTALLFILE_BIN_LOCAL = $(STRIP) --strip-unneeded $(1) \
                        -o $(INSTALLDIR_LOCAL)/$(1)

#
# Use Xorg headers as supplied with L4con
#

PRIVATE_INCDIR = $(SRC_DIR) $(X11_INCDIR)

#
# Use L4cons implementation of emul_l4env.c
#

vpath emul_l4env.c $(CONDRV_DIR)

include $(L4DIR)/mk/prog.mk
