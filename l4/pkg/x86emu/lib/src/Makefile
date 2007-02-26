PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libx86emu.a
MODE		= sigma0
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= decode.c fpu.c ops.c ops2.c prim_ops.c sys.c
CPPFLAGS	= -DNO_LONG_LONG
CFLAGS		= -fomit-frame-pointer
PRIVATE_INCDIR	= ../../include ..
WARNINGS	= -Wall -Wstrict-prototypes

DEBUG 		= 0

include $(L4DIR)/mk/lib.mk
