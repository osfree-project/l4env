SYSTEMS		:= x86-l4v2 x86-l4x0 arm-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= sigma0

ifneq ($(filter $(OSYSTEM), x86-l4x0 x86-l4v2),)
MODE		= sigma0
LIBCLIBS	= -nostdlib $(LIBCLIBS_X86) $(GCCLIB)
L4LIBS		= -ll4sys
CRT0		=
LDSCRIPT	= $(PKGDIR)/server/src/ARCH-x86/sigma0.ld
else
MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
L4LIBS		= -lcxx_start -lcxx_thread -lcxx_io -lcxx_io_kdebug -lcxx_base 
LIBCINCDIR	= -nostdinc
#LIBCBACKEND_LIB	= -ldiet_be_minimal_io -ldiet_be_l4_start_stop
DEFAULT_RELOC_arm  = 0x00038000
endif

PRIVATE_INCDIR	= . ..

SRC_CC_arm-l4x0 = $(addprefix ARCH-$(ARCH)/, main.cc memmap.cc)
SRC_C_x86-l4x0	= $(addprefix ARCH-$(ARCH)/, $(SUPPORT_FILE) globals.c init.c memmap.c)
SRC_C_x86-l4v2	= $(SRC_C_x86-l4x0)
SRC_S_x86-l4x0	= $(addprefix ARCH-$(ARCH)/, crt0.S)
SRC_S_x86-l4v2	= $(SRC_S_x86-l4x0)

CXXFLAGS	+= -fno-rtti

include $(L4DIR)/mk/prog.mk

ifeq ($(USE_DIETLIBC),y)
SUPPORT_FILE	= libc_support.c
LIBCLIBS_X86	= -ldietlibc_support -ldiet_c \
		  -ldiet_be_minimal_io -ll4util_light
else
ifeq ($(USE_UCLIBC),y)
SUPPORT_FILE	= libc_support.c
LIBCLIBS_X86	= -luclibc_support -luc_c -luc_be_minimal_io -ll4util_light
else
SUPPORT_FILE	= oskit_support.c
LIBCLIBS_X86	= -lkern -lmc
endif
endif
