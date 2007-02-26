SYSTEMS         := x86-l4v2 x86-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
vpath %.c	$(PKGDIR)/server/src
vpath %.S	$(PKGDIR)/server/src
DEFINES		+= -DFIASCO_UX

TARGET		= sigma0-ux
MODE		= sigma0
LIBCLIBS	= -nostdlib $(LIBCLIBS_X86) $(GCCLIB)
L4LIBS		= -ll4sys

PRIVATE_INCDIR 	= $(PKGDIR)/server/src/ARCH-x86 $(PKGDIR)/server/src
SRC_C		= $(addprefix ARCH-$(ARCH)/, $(SUPPORT_FILE) globals.c init.c memmap.c)
SRC_S		= $(addprefix ARCH-$(ARCH)/, crt0.S)
CRT0 		=
LDSCRIPT	= $(PKGDIR)/server/src/ARCH-x86/sigma0.ld

include $(L4DIR)/mk/prog.mk

ifeq ($(USE_DIETLIBC),y)
SUPPORT_FILE    = libc_support.c
LIBCLIBS_X86    = -ldietlibc_support -ldiet_c \
		  -ldiet_be_minimal_io -ll4util_light
else
ifeq ($(USE_UCLIBC),y)
SUPPORT_FILE    = libc_support.c
LIBCLIBS_X86    = -luclibc_support -luc_c -luc_be_minimal_io -ll4util_light
else
SUPPORT_FILE    = oskit_support.c
LIBCLIBS_X86    = -lkern -lmc
endif
endif
