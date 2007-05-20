PKGDIR     ?= ../..
L4DIR      ?= $(PKGDIR)/../..
CONDRV_DIR  = $(L4DIR)/pkg/l4con/examples/xf86_stub

TARGET     = ovlscreen_drv.so
SRC_C      = ovlscreen.c emul_l4env.c
MODE       = linux
LIBS       = -ll4ovlscreen.p -ldm_generic.p -ldm_mem.p -lnames.p \
             -ll4util.p $(ROOTPLIB) -ll4sys.p
LIBCINCDIR = $(addprefix -I,$(L4INCDIR) $(GCCINCDIR))
LDFLAGS    = -nostdlib -nostartfiles -shared
LDSCRIPT   = $(call findfile,main_rel.ld,$(L4LIBDIR))
CRT0       = $(CRTP_DEFAULT)
CFLAGS     = -fPIC
CPPFLAGS   = -nostdinc

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

PRIVATE_INCDIR = $(SRC_DIR) $(CONDRV_DIR) $(CONDRV_DIR)/include-xorg-7.1.0

#
# Use L4con's implementation of emul_l4env.c
#

vpath emul_l4env.c $(CONDRV_DIR)

include $(L4DIR)/mk/prog.mk

