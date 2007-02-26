#
# libavutil Makefile
#
include ../../video/libavcodec/config.mak

# NOTE: -I.. is needed to include config.h
CFLAGS=$(OPTFLAGS) -DHAVE_AV_CONFIG_H -DBUILD_AVUTIL -I$(PKGDIR_OBJ)/config -I../../video/libavcodec $(LIBCINCDIR_x86_tiny) -D_GNU_SOURCE -Wall

#FIXME: This should be in configure/config.mak
ifeq ($(CONFIG_WIN32),yes)
    LDFLAGS=-Wl,--output-def,$(@:.dll=.def),--out-implib,lib$(SLIBNAME:$(SLIBSUF)=.dll.a)
endif

OBJS= mathematics.o \
      integer.o \
      rational.o \
      intfloat_readwrite.o \
      crc.o \

HEADERS = avutil.h common.h mathematics.h integer.h rational.h \
          intfloat_readwrite.h

ifeq ($(TARGET_ARCH_SPARC64),yes)
CFLAGS+= -mcpu=ultrasparc -mtune=ultrasparc
endif

NAME=avutil
SUBDIR = libavutil
ifeq ($(BUILD_SHARED),yes)
LIBVERSION=$(LAVUVERSION)
LIBMAJOR=$(LAVUMAJOR)
endif

include ../../video/libavcodec/common.mak
