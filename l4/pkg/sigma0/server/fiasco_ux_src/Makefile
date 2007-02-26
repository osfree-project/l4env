SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
vpath %.c	$(PKGDIR)/server/src
vpath %.S	$(PKGDIR)/server/src
DEFINES		+= -DFIASCO_UX

TARGET		= sigma0-ux
MODE		= sigma0
LIBCLIBS	= -nostdlib -lkern -lmc $(GCCLIB)
L4LIBS		= -ll4sys

PRIVATE_INCDIR 	= $(PKGDIR)/server/src/ARCH-x86 $(PKGDIR)/server/src
SRC_C		= $(addprefix ARCH-$(ARCH)/, oskit_support.c globals.c init.c memmap.c)
SRC_S		= $(addprefix ARCH-$(ARCH)/, crt0.S)
CRT0 		=
LDSCRIPT	= $(PKGDIR)/server/src/ARCH-x86/sigma0.ld

include $(L4DIR)/mk/prog.mk
