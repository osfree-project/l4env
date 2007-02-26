# Makefile for LAME 3.xx
#
# LAME is reported to work under:  
# Linux (i86), NetBSD 1.3.2 (StrongARM), FreeBSD (i86)
# Compaq Alpha(OSF, Linux, Tru64 Unix), Sun Solaris, SGI IRIX,
# OS2 Warp, Macintosh PPC, BeOS, Amiga and even VC++ 
# 

#cr7
#UNAME = $(shell uname)
#ARCH = $(shell uname -m)

ARCH = i686
UNAME = Linux

# defaults:
PGM = lame
CC = gcc
CC_OPTS =  -O
MAKEDEP = -M
RM = rm -f


##########################################################################
# LINUX   
##########################################################################
ifeq ($(UNAME),Linux)


CC_OPTS =  -O3 -fomit-frame-pointer -funroll-loops -ffast-math  -finline-functions -Wall


endif

# 10/99 added -D__NO_MATH_INLINES to fix a bug in *all* versions of
# gcc 2.8+ as of 10/99.  

CC_SWITCHES = -DNDEBUG -D__NO_MATH_INLINES $(CC_OPTS) $(SNDLIB) 
c_sources = \
        brhist.c \
	formatBitstream.c \
	fft.c \
	l3bitstream.c \
        id3tag.c \
	ieeefloat.c \
        newmdct.c \
        parse.c \
	portableio.c \
	psymodel.c \
	quantize.c \
	quantize-pvt.c \
	vbrquantize.c \
	reservoir.c \
	tables.c \
	takehiro.c \
	timestatus.c \
	util.c \
        version.c \
        lame.c \

OBJ = $(c_sources:.c=.o)

%.o: %.c 
	$(CC) $(CPP_OPTS) $(CC_SWITCHES) -c $< -o $@

all: libmp3lame.a

libmp3lame.a:  $(OBJ)
	ar cr ../../libmp3lame.a  $(OBJ) 

clean:
	-$(RM) $(OBJ) $(DEP) $(PGM) main.o rtp.o mp3rtp mp3rtp.o \
         mp3x.o mp3x ../../libmp3lame.a 

cleanall:
	$(RM) $(OBJ) $(DEP) $(PGM) main.o rtp.o mp3rtp mp3rtp.o \
         mp3x.o mp3x ../../libmp3lame.a 

install:

tags: TAGS

TAGS: ${c_sources}
	etags -T ${c_sources}

