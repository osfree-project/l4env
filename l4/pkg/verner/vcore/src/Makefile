# directories we need to know
PKGDIR ?=	..
L4DIR ?=	$(PKGDIR)/../../..

# include configuration
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/../Makeconf.bid.local

VPATH += $(PKGDIR)/../plugins/codec
VPATH += $(PKGDIR)/../plugins/filter
VPATH += $(PKGDIR)/../common

# target
TARGET =	vtcore_video vtcore_audio
# vcore_aud
SYSTEMS =	x86-l4v2

# common include dirs
PRIVATE_INCDIR  =  \
		  $(PKGDIR)/../common \
		  $(PKGDIR)/../lib/include \
		  $(PKGDIR_OBJ)/../lib/include \
		  $(PKGDIR)/../lib/support/libavutil \
		  $(PKGDIR)/include \
		  $(PKGDIR)/../include \
	          $(PKGDIR_OBJ)/../config \
		  $(PKGDIR)/../plugins/import \
		  $(PKGDIR)/../plugins/filter \
		  $(PKGDIR)/../plugins/codec

PRIVATE_LIBDIR += $(PKGDIR_OBJ)/../lib/video/postproc

# common source files
SRC_C = receiver.c timer.c minstances.c sender.c work_loop.c \
	work_loop_audio.c work_loop_video.c

#RT Support
ifeq ($(BUILD_RT_SUPPORT),y)
SRC_C		+= qap_rt.c
else
SRC_C		+= qap.c
endif

ifeq ($(RT_USE_CPU_RESERVE),y)
LIBS		+= -lcpu_reserve
endif

#--------------------------
# video specific
#--------------------------
SRC_C_vtcore_video = \
		video_request_server.c video_startup.c video_codecs.c \
		video_postproc.c dummy.c

# Plugins
SRC_C_vtcore_video += vc_pass.c vf_libpostproc.c

# define we compile as VCORE for video
DEFINES_video_request_server.c = -DUSE_VCORE_VID

SERVERIDL_vtcore_video = vcore_video.idl
LIBS_vtcore_video += -ldsi -lpostproc -lx86cpucaps -laclib


ifeq ($(BUILD_libmpeg2),y)
SRC_C_vtcore_video += vc_mpeg.c  
LIBS_vtcore_video += -lmpeg2
endif
ifeq ($(BUILD_libavcodec),y)
SRC_C_vtcore_video += vc_ffmpeg.c
LIBS_vtcore_video += -lavcodec -lavutil
PRIVATE_LIBDIR += \
	$(PKGDIR_OBJ)/../lib/video/libavcodec \
	$(PKGDIR_OBJ)/../lib/support/libavutil
endif
ifeq ($(BUILD_xvid10),y)
SRC_C_vtcore_video += vc_xvid_10.c  
LIBS_vtcore_video += -lxvidcore
PRIVATE_LIBDIR += $(PKGDIR_OBJ)/../lib/video/xvidcore-1.0/build
endif		

#--------------------------
# audio specific
#--------------------------
SRC_C_vtcore_audio = \
		audio_request_server.c audio_startup.c audio_codecs.c audio_postproc.c

# define we compile as VCORE for audio
DEFINES_audio_request_server.c = -DUSE_VCORE_AUD

SERVERIDL_vtcore_audio = vcore_audio.idl

LIBS_vtcore_audio = -ldsi -lx86cpucaps

# Plugins
SRC_C_vtcore_audio +=  ac_pass.c

ifeq ($(BUILD_mpg123),y)
SRC_C_vtcore_audio += ac_mp3.c 
LIBS_vtcore_audio += -lmpg123
# use petze ?
ifeq ($(MPG123LIB_PETZE),y)
LIBS_vtcore_audio += -lpetze
endif
endif

ifeq ($(BUILD_lame),y)
SRC_C_vtcore_audio += ac_lame.c
LIBS_vtcore_audio += -lmp3lame
endif


# reloc addr
DEFAULT_RELOC_vtcore_video = $(VCORE_VIDEO_RELOC)
DEFAULT_RELOC_vtcore_audio = $(VCORE_AUDIO_RELOC)

# use petze ?
ifeq ($(VCORE_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif

#use rt_mon ?
ifeq ($(VCORE_VIDEO_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif
ifeq ($(VCORE_AUDIO_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif
ifeq ($(VCORE_FILTER_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif
ifeq ($(RTMON_DSI_BENCHMARK),y)
LIBS	       += -lrt_mon_client
endif

# use decoding time prediction ?
ifeq ($(PREDICT_DECODING_TIME),y)
MODE		= l4env_base
LIBS	       += -lpredict
endif
ifeq ($(PREDICT_DECODING_TIME_RTMON),y)
LIBS	       += -lrt_mon_client
endif

ifeq ($(H264_SLICE_SCHEDULE),y)
MODE		= l4env_base
PRIVATE_INCDIR += $(PKGDIR)/../lib/h264_schedule $(PKGDIR)/../lib/video/libavcodec
CPPFLAGS       += -DVERNER
LIBS	       += -lh264sched
endif

# include prog role
include $(L4DIR)/mk/prog.mk

# depend on configuration files
Makefile.inc: $(PKGDIR_OBJ)/../Makeconf.bid.local
