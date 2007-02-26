# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../../..

# source files
SRC_C =		player.c \
		startup.c \
		minstances.c \
		helper.c

SERVERIDL = 	vcontrol-remote.idl

# target
TARGET =	vrcontrol
SYSTEMS =	x86-l4v2

# include configuration
-include $(PKGDIR)/../Makeconf.bid.local
DEFAULT_RELOC = $(VCONTROL_RELOC)

VPATH += $(PKGDIR)/../common

vpath helper.c $(PKGDIR)/../vcontrol/src

LIBS            =   \
		    -lvtsync_client \
		    -lvtdemuxer_client \
		    -lvtcore_video_client \
		    -lvtcore_audio_client \
		    -ldsi -lverner_gui

PRIVATE_INCDIR  =  \
		  $(PKGDIR)/../common \
		  $(PKGDIR)/../include \
		  $(PKGDIR)/../plugins/import $(PKGDIR)/../plugins/codec \
                  $(PKGDIR)/../config \
		  $(PKGDIR)/../vcontrol/src

# include prog role
include $(L4DIR)/mk/prog.mk

# depend on configuration files
$(TARGET): $(PKGDIR)/../Makeconf.bid.local
