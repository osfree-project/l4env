SYSTEMS		:= x86-l4v2 x86-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= sigma0
MODE		= sigma0
LIBCLIBS	= -nostdlib -lkern -lmc
L4LIBS		= -ll4sys
PRIVATE_INCDIR	= .

SRC_C		= oskit_support.c globals.c init.c memmap.c
SRC_S		= crt0.S
CRT0 		=
LDSCRIPT	= $(PKGDIR)/server/src/sigma0.ld

include $(L4DIR)/mk/prog.mk
