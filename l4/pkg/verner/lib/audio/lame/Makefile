SYSTEMS := x86-l4v2
PKGDIR  ?= ../../..
L4DIR   ?= $(PKGDIR)/../..

TARGET   = libmp3lame.a

CFLAGS   = -O3 -fomit-frame-pointer -funroll-loops -ffast-math  -finline-functions -Wall
CPPFLAGS = -DNDEBUG -D__NO_MATH_INLINES -DNO_TIME_DSP

SRC_C    = \
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
	lame.c

include $(L4DIR)/mk/lib.mk
