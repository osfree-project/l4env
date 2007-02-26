# directories we need to know
PKGDIR ?=	..
L4DIR ?=	$(PKGDIR)/../../..

# target
TARGET = 	libvtcore_video_client.a libvtcore_audio_client.a
SYSTEMS =	x86-l4v2 

# video source files
SRC_C_libvtcore_video_client.a =		clientlib_video.c 
CLIENTIDL_libvtcore_video_client.a =	vcore_video.idl 

# audio source files
SRC_C_libvtcore_audio_client.a =		clientlib_audio.c 
CLIENTIDL_libvtcore_audio_client.a =	vcore_audio.idl 

PRIVATE_INCDIR = $(PKGDIR)/../include $(PKGDIR_OBJ)/../config

# include lib role
include $(L4DIR)/mk/lib.mk
