L4DIR ?= ../../../../..
include $(L4DIR)/Makeconf.bid.local

CC = $(CC_x86)

#include ../config.mak

SWSLIB = libswscale.a
ifeq ($(SHARED_PP),yes)
SPPLIB = libpostproc.so
SPPVERSION = 0.0.1
endif
PPLIB = ../../libpostproc.a

SWSSRCS=swscale.c rgb2rgb.c yuv2rgb.c
SWSOBJS=$(SWSSRCS:.c=.o)
PPOBJS=postprocess.o
SPPOBJS=postprocess_pic.o
CS_TEST_OBJS=cs_test.o rgb2rgb.o ../cpudetect.o ../mp_msg.o ../libvo/aclib.o

OPTFLAGS += -O4 -march=i686 -mcpu=i686 -pipe -ffast-math -fomit-frame-pointer -D_REENTRANT 
#OPTFLAGS += -O4 -march=athlon-4 -mcpu=athlon-4 -pipe -ffast-math -fomit-frame-pointer -D_REENTRANT 

CFLAGS  = $(OPTFLAGS) $(MLIB_INC) -I. -I../.. $(EXTRA_INC)
ifeq ($(USE_DIETLIBC),y)
CFLAGS += -DUSE_DIETLIBC=y
endif
# -I/usr/X11R6/include/

.SUFFIXES: .c .o

# .PHONY: all clean

.c.o:
	$(CC) -c $(CFLAGS) -o $@ $<

all:    $(PPLIB)

$(SWSLIB):     $(SWSOBJS)
	$(AR) r $(SWSLIB) $(SWSOBJS)

clean:
	rm -f *.o *.a *~ *.so $(PPLIB)

distclean:
	rm -f Makefile.bak *.o *.a *~ *.so .depend

cleanall: clean
	rm -f Makefile.bak *.o *.a *~ *.so .depend


dep:    depend

depend:
	$(CC) -MM $(CFLAGS) $(SWSSRCS) postprocess.c 1>.depend

cs_test: $(CS_TEST_OBJS)
	$(CC) $(CS_TEST_OBJS) -o cs_test

ifeq ($(SHARED_PP),yes)
postprocess_pic.o: postprocess.c
	$(CC) -c $(CFLAGS) -fomit-frame-pointer -fPIC -DPIC -o $@ $<

$(SPPLIB): $(SPPOBJS)
	$(CC) -shared -Wl,-soname,$(SPPLIB).0 \
	-o $(SPPLIB) $(SPPOBJS)
endif

$(PPLIB): $(PPOBJS)
	$(AR) r $(PPLIB) $(PPOBJS)

install: all


#
# include dependency files if they exist
#
ifneq ($(wildcard .depend),)
include .depend
endif
