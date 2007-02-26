prefix=/usr/local
bindir=/usr/local/bin
mandir=/usr/local/man
MAKE=make
CC=gcc
AR=ar
RANLIB=ranlib
STRIP=strip
OPTFLAGS= -O3
LDFLAGS=-Wl,--warn-common -rdynamic
FFSLDFLAGS=-Wl,-E
SHFLAGS=-shared
LIBPREF=../../lib
LIBSUF=.a
SLIBPREF=lib
SLIBSUF=.so
EXESUF=
TARGET_OS=Linux
TARGET_ARCH_X86=yes
TARGET_MMX=yes
HAVE_IMLIB2=no
HAVE_FREETYPE2=no
CONFIG_SDL=no
EXTRALIBS=-lm
VERSION=0.4.8
SRC_PATH=.


-include $(PKGDIR)/Makeconf.bid.local

#if BUILD_libavcodec_encoder
CONFIG_ENCODERS=yes
#endif
#if BUILD_libavcodec_decoder
CONFIG_DECODERS=yes
#endif
#if BUILD_libavcodec_risky
CONFIG_RISKY=yes
#endif

# reenable to build w/ SSE
# TARGET_BUILTIN_VECTOR=yes 
