PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifneq ($(SYSTEM),)
ifeq ($(GCCVERSION),2.95)
TARGET		= libdropscon.a
else
all install::
	@echo -e $(EMPHSTART)"  ==> Console stub library is not built since Linux-2.2 code requires gcc-2.95!"$(EMPHSTOP)
endif
endif
MODE		= l4linux_kern
SYSTEMS		= x86-l4v2
WARNINGS	= -Wno-missing-prototypes -Wno-missing-declarations

SRC_C		= main.c comh.c kbd.c xf86if.c bootlog.c #logif.c
vpath %.c	../../linux_stub

include $(L4DIR)/mk/lib.mk
