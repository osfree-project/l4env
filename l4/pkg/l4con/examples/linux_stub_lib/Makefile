PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libdropscon.a
MODE		= l4linux_kern
SYSTEMS		= x86-l4v2 x86-l4x0
WARNINGS	= -Wno-missing-prototypes -Wno-missing-declarations

SRC_C		= main.c comh.c kbd.c xf86if.c bootlog.c #logif.c
vpath %.c	../../linux_stub

include $(L4DIR)/mk/lib.mk
