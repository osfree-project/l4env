PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dropscon.o
MODE		= l4linux_kern
DEFAULT_RELOC	= 0x00800000

SRC_C		= main.c comh.c kbd.c xf86if.c bootlog.c #logif.c
CPPFLAGS	= -DMODULE
LIBS		= -lcon -lnames -lconstream-server
LDFLAGS		= -r -nostdlib -nostartfiles
LDSCRIPT	= $(call findfile,main_rel.ld,$(L4LIBDIR))

WARNINGS	= -Wall -Wstrict-prototypes

include $(L4DIR)/mk/prog.mk
