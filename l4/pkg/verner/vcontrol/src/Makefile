# directories we need to know
PKGDIR ?=	..
L4DIR ?=	$(PKGDIR)/../../..
DOPE_DIR        = $(L4DIR)/pkg/dope

# source files
SRC_C =		\
		player-UI.c \
		extctrl.c \
		playlist.c \
		helper.c \
		startup.c \
		mutex.c \
		minstances.c \

# target
TARGET =	vtcontrol
SYSTEMS =	x86-l4v2

# include configuration
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/../Makeconf.bid.local
DEFAULT_RELOC = $(VCONTROL_RELOC)

VPATH += $(PKGDIR)/../common


LIBS            =   \
		    -lvtsync_client \
		    -lvtdemuxer_client \
		    -lvtcore_video_client \
		    -lvtcore_audio_client \
                    -ll4dope  \
		    -ldsi \
		    

PRIVATE_INCDIR  = $(SRC_DIR) . \
		  $(PKGDIR)/../common \
		  $(PKGDIR)/../include \
		  $(PKGDIR)/../plugins/import $(PKGDIR)/../plugins/codec \
                  $(PKGDIR_OBJ)/../config \
                  $(DOPE_DIR)/include \
                  $(DOPE_DIR)/server/include \
		  $(OBJ_DIR) \

#use petze ?
ifeq ($(VCONTROL_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif
# include prog role
include $(L4DIR)/mk/prog.mk

# ui
SRC_DPE	= player-UI.dpe
player-UI.o: player-UI.dpi

# depend on configuration files
$(TARGET): $(PKGDIR_OBJ)/../Makeconf.bid.local

