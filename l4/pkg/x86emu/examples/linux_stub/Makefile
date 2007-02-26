PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= x86emu.o
MODE		= l4linux_kern
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= emu.c
LIBS		= -lx86emu
CPPFLAGS	= -DMODULE -fomit-frame-pointer
WARNINGS	= -Wall -Wstrict-prototypes
LDFLAGS		= -r -nostdlib -nostartfiles

INSTALLFILE_BIN_LOCAL = $(STRIP) --strip-unneeded $(1) \
			-o $(INSTALLDIR_LOCAL)/$(TARGET)

include $(L4DIR)/mk/prog.mk
