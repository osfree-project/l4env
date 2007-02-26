PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifneq ($(SYSTEM),)
ifeq ($(GCCVERSION),2.95)
TARGET		= dropscon.o
else
all install::
	@echo -e $(EMPHSTART)"  ==> Console stub module is not built since Linux-2.2 code requires gcc-2.95!"$(EMPHSTOP)
endif
endif

MODE		= l4linux_kern
DEFAULT_RELOC	= 0x00800000

SRC_C		= main.c comh.c kbd.c xf86if.c bootlog.c
CPPFLAGS	= -DMODULE
LIBS		= -lcon -lnames -lconstream-server
LDFLAGS		= -r -nostdlib -nostartfiles
LDSCRIPT	= $(call findfile,main_rel.ld,$(L4LIBDIR))

WARNINGS	= -Wall -Wstrict-prototypes

include $(L4DIR)/mk/prog.mk
