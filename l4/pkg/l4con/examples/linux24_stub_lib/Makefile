PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= liblinux24-con-stub.a
MODE		= l4linux_kern
WARNINGS	= -Wno-missing-prototypes -Wno-missing-declarations

SRC_C		= main.c comh.c kbd.c xf86if.c bootlog.c #logif.c
vpath %.c	../../linux_stub
LIBCINCDIR	= -nostdinc -I$(LINUX24_INCDIR) -D__KERNEL__ -DLINUX_ON_L4 -I$(GCCINCDIR) 

include $(L4DIR)/mk/lib.mk
