PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libx86emu_int10.a libx86emu_int10-ux.a
SYSTEMS		= x86-l4v2
MODE		= l4env
SRC_C_libx86emu_int10.a    = int10.c
SRC_C_libx86emu_int10-ux.a = int10-ux.c
CFLAGS		= -fomit-frame-pointer

include $(L4DIR)/mk/lib.mk
