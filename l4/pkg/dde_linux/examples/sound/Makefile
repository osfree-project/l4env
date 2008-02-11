PKGDIR     ?= ../..
L4DIR      ?= $(PKGDIR)/../..

SYSTEMS     = x86-l4v2

TARGET      = snddrv-es1370 snddrv-es1371 snddrv-i810
DEFAULT_RELOC = 0x00800000

SRC_C       = server.c

VPATH += $(PKGDIR)/examples/sound/contrib
SRC_C_snddrv-es1370 = es1370.c ac97_codec.c
SRC_C_snddrv-es1371 = es1371.c ac97_codec.c
SRC_C_snddrv-i810 = i810_audio.c ac97_codec.c

### specialties
PRIVATE_INCDIR = $(DDE_INCDIR) \
                 $(PKGDIR)/examples/sound/contrib $(LINUX24_INCDIR)
LIBCINCDIR  = -nostdinc $(I_GCCINCDIR)

LIBS = -ldde_linux -ldde_linux-sound -lio -lomega0 -llist_alloc

DEFINES                 = -D__KERNEL__ -DDDE_LINUX
DEFINES_i810_audio.c    = -UDEBUG
DEFINES_es1370.c        = -UDEBUG
DEFINES_es1371.c        = -UDEBUG
DEFINES_ac97_codec.c    = -UDEBUG
WARNINGS = -Wall -Wstrict-prototypes -fno-strict-aliasing

# XXX this looks ugly
-include $(PKGDIR)/examples/sound/Makeconf.local

# If you have complete Linux sources use this in Makeconf.local:
#
# MY_DRV_PATH = LINUX_DIR/drivers/sound
# MY_DRV_FILES = <files>
# MY_DRV_NAME = <name>
#
# You can compile your own drivers that way
ifneq ($(strip $(MY_DRV_NAME)),)
TARGET += snddrv-$(MY_DRV_NAME)
VPATH += $(MY_DRV_PATH)
SRC_C_snddrv-$(MY_DRV_NAME) = $(MY_DRV_FILES)
endif

### act the role of ...
include $(L4DIR)/mk/prog.mk
