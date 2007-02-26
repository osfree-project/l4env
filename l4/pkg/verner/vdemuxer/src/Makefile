
SYSTEMS         = x86-l4v2
PKGDIR          ?= ..
L4DIR 		?= $(PKGDIR)/../../..

# include configuration
-include $(PKGDIR)/../Makeconf.bid.local
DEFAULT_RELOC = $(VDEMUXER_RELOC)

TARGET 		= vtdemuxer 

VPATH += $(PKGDIR)/../plugins/import
VPATH += $(PKGDIR)/../plugins/io
VPATH += $(PKGDIR)/../common

SERVERIDL = vdemuxer.idl

#GENERAL FILES
SRC_C		= startup.c \
		  container.c \
		  work_loop.c \
		  request_server.c \
		  minstances.c \
		  fileops.c \
		  timer.c 

DEFINES         = -DUSE_VDEMUXER

#RT Support
ifeq ($(RT_USE_CPU_RESERVE),y)
LIBS		+= -lcpu_reserve
endif

#DEMUXER-PLUGINS
ifeq ($(VDEMUXER_BUILD_OGMLIB),y)
SRC_C		+= demux_ogm.c
LIBS		+= -logmlib
endif
ifeq ($(VDEMUXER_BUILD_AVILIB),y)
SRC_C		+= demux_avi.c 
LIBS		+= -lavilib
endif
ifeq ($(VDEMUXER_BUILD_MPEG),y)
SRC_C		+= demux_mpg.c 
endif
ifeq ($(VDEMUXER_BUILD_MP3),y)
SRC_C		+= demux_mp3.c  
endif
#
# not ready yet
#ifeq ($(VDEMUXER_BUILD_LIBAVFORMAT),y)
#LIBS		+= -lavformat -lavcodec \
#endif

#I/O-PLUGINS
ifeq ($(VDEMUXER_BUILD_GRUBFS),y)
SRC_C		+= io_grubfs.c
endif
ifeq ($(VDEMUXER_BUILD_EXT2FS),y)
SRC_C		+= io_ext2fs.c
LIBS	 	+= -ll4ext2fs
endif
ifeq ($(VDEMUXER_BUILD_RTNETFS),y)
SRC_C		+= io_rtns.c
LIBS	 	+= -loshk-rtns -loshkoshnicdrv
endif

#LIBS
LIBS +=		-ldsi -lparsecmdline \
		-lprobelib -lmpg123\
		-lcr7libc -lcr7libm


PRIVATE_LIBDIR  = $(PKGDIR)/../lib/

#INCLUDES
PRIVATE_INCDIR  =  \
		  $(PKGDIR)/../common \
		  $(PKGDIR)/../lib/include \
		  $(PKGDIR)/include \
		  $(PKGDIR)/../include \
	          $(PKGDIR)/../config \
		  $(PKGDIR)/../plugins/import \
		  $(PKGDIR)/../plugins/io \
		  $(PKGDIR)/../lib/de-muxer/libavformat

#use petze ?
ifeq ($(VDEMUXER_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif
ifeq ($(OGMLIB_PETZE),y) 
LIBS += -lpetze
endif
ifeq ($(AVILIB_PETZE),y) 
LIBS += -lpetze
endif
ifeq ($(MPG123LIB_PETZE),y)
LIBS += -lpetze
endif

#use rt_mon ?
ifeq ($(VDEMUXER_VIDEO_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif
ifeq ($(VDEMUXER_AUDIO_BENCHMARK),y)
LIBS           += -lrt_mon_client
endif
ifeq ($(RTMON_DSI_BENCHMARK),y)
LIBS	       += -lrt_mon_client
endif


#default is prog
include $(L4DIR)/mk/prog.mk

#depend on configuration files
$(TARGET): $(PKGDIR)/../Makeconf.bid.local

Makefile: $(PKGDIR)/../Makeconf.bid.local
