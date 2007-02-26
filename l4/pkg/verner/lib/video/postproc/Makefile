
include ../libavcodec/config.mak

# Overload incdir, postproc include files go in a different directory.
incdir=$(prefix)/include/postproc

NAME=postproc
SUBDIR=postproc
ifeq ($(BUILD_SHARED),yes)
LIBVERSION=$(SPPVERSION)
LIBMAJOR=$(SPPMAJOR)
endif

OBJS=postprocess.o

HEADERS = postprocess.h

CFLAGS  = $(OPTFLAGS) $(MLIB_INC) -I. -I$(PKGDIR_OBJ)/config -I../libavcodec $(LIBCINCDIR_x86_tiny) $(EXTRA_INC)
# -I/usr/X11R6/include/

include ../libavcodec/common.mak

ifeq ($(BUILD_SHARED),yes)
postprocess_pic.o: postprocess.c
	$(CC) -c $(CFLAGS) -fomit-frame-pointer -fPIC -DPIC -I.. -I../.. -o $@ $<
endif

