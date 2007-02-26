
SYSTEMS         = x86-l4v2
PKGDIR          ?= ..
L4DIR 		?= $(PKGDIR)/../../..
DOPE_DIR	= $(L4DIR)/pkg/dope

# include configuration
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/../Makeconf.bid.local
DEFAULT_RELOC = $(VSYNC_RELOC)

ifeq ($(VSYNC_ES1371_AOUT),y)
TARGET          +=  vtsync-es1371 
endif
ifeq ($(VSYNC_I810_AOUT),y)
TARGET          +=  vtsync-i810
endif
ifeq ($(VSYNC_VIA_AOUT),y)
TARGET          +=  vtsync-via
endif
ifeq ($(VSYNC_NO_AOUT),y)
TARGET          +=  vtsync-noaudio
endif


VPATH += $(PKGDIR)/../plugins/export
VPATH += $(PKGDIR)/../plugins/fx
VPATH += $(PKGDIR)/src/contrib
VPATH += $(PKGDIR)/../common

SERVERIDL = vsync.idl

#GENERAL FILES

SRC_C		= startup.c \
		  request_server.c \
		  metronome.c \
		  playerstatus.c \
		  minstances.c \
		  timer.c \
		  mutex.c \
		  work_loop.c \
		  work_loop_audio.c \
		  work_loop_video.c

DEFINES         = -DUSE_VSYNC 

#RT Support
ifeq ($(RT_USE_CPU_RESERVE),y)
LIBS		+= -lcpu_reserve
endif

#VIDEO SETTINGS
SRC_C           +=  osd.c vo_dope.c 
LIBS            +=  -ll4dope -lvscreen 
ifeq ($(VSYNC_BUILD_WITH_CON),y)
SRC_C           +=  vo_con.c
LIBS            +=  -lcon
endif

LIBS            +=   \
		  -ldsi \
		  $(GCCLIB) \
		  -lx86cpucaps -laclib

PRIVATE_INCDIR  = $(SRC_DIR)/ $(SRC_DIR)/.. \
		  $(PKGDIR)/../common \
		  $(PKGDIR)/../lib/include \
		  $(PKGDIR_OBJ)/../lib/include \
		  $(PKGDIR)/include \
		  $(PKGDIR)/../include \
	          $(PKGDIR_OBJ)/../config \
		  $(PKGDIR)/../plugins/export \
		  $(PKGDIR)/../plugins/fx \
		  $(DOPE_DIR)/include


#AUDIO SETTING
PRIVATE_INCDIR  += \
		  $(PKGDIR)/src/contrib \
		  $(DDE_INCDIR) \
	          $(LINUX24_INCDIR)

LIBS_vtsync-es1371 += -ldde_linux -ldde_linux-sound -lio -lomega0 -llist_alloc
LIBS_vtsync-i810   += -ldde_linux -ldde_linux-sound -lio -lomega0 -llist_alloc
LIBS_vtsync-via    += -ldde_linux -ldde_linux-sound -lio -lomega0 -llist_alloc

DEFINES                += -D__KERNEL__ -DDDE_LINUX 
DEFINES_i810.c          = -UDEBUG
DEFINES_es1371.c        = -UDEBUG
DEFINES_ac97_codec.c    = -UDEBUG
DEFINES_via82cxx_audio.c    = -UDEBUG
WARNINGS        = -Wall -Wstrict-prototypes -fno-strict-aliasing

ifeq ($(BUILD_goom),y)
SRC_C           +=  fx_goom.c
LIBS            +=  -lfxgoom -lSDL -ldsound
endif

SRC_C_vtsync-es1371 = contrib/es1371.c contrib/ac97_codec.c ao_oss.c
SRC_C_vtsync-i810   = contrib/i810_audio.c contrib/ac97_codec.c ao_oss.c 
SRC_C_vtsync-via    = contrib/via82cxxx_audio.c dev_table.c contrib/ac97_codec.c ao_oss.c 
SRC_C_vtsync-noaudio = ao_null.c 


ifneq ($(strip $(MY_DRV_NAME)),)
TARGET += vtsync-$(MY_DRV_NAME)
VPATH += $(MY_DRV_PATH)
SRC_C_$(MY_DRV_NAME) = $(MY_DRV_FILES)
endif

#use petze ?
ifeq ($(VSYNC_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif
ifeq ($(GOOM_PETZE),y) 
LIBS += -lpetze
endif

#use rt_mon ?
ifeq ($(VSYNC_VIDEO_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif
ifeq ($(VSYNC_AUDIO_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif
ifeq ($(RTMON_DSI_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif


#default is prog
include $(L4DIR)/mk/prog.mk

DEFINES		+= -DMODVERSIONS -DEXPORT_SYMTAB

ifeq ($(LINUX24_INCDIR),)
  $(error Please set setup path to Linux 2.4.20 headers (LINUX24_INCDIR) in $(L4DIR)/Makeconf.local)
endif

Makefile.inc: $(PKGDIR_OBJ)/../Makeconf.bid.local
