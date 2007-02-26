SYSTEMS		:= x86-l4v2 x86-l4x0 arm-l4x0
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= sigma0

ifneq ($(filter $(OSYSTEM), x86-l4x0 x86-l4v2),)
MODE		= sigma0
LIBCLIBS	= -nostdlib -lkern -lmc $(GCCLIB)
L4LIBS		= -ll4sys
CRT0		=
LDSCRIPT	= $(PKGDIR)/server/src/ARCH-x86/sigma0.ld
else
MODE		= libc
LIBCLIBS	= -nostdlib $(GCCLIB)
L4LIBS		= -lcxx_start -lcxx_thread -lcxx_io -lcxx_io_kdebug -lcxx_base
LIBCINCDIR	= -nostdinc
#LIBCBACKEND_LIB	= -ldiet_be_minimal_io -ldiet_be_l4_start_stop
DEFAULT_RELOC	= 0xc0080000
endif

PRIVATE_INCDIR	= . ..

SRC_CC_arm-l4x0 = $(addprefix ARCH-$(ARCH)/, main.cc memmap.cc)
SRC_C_x86-l4x0	= $(addprefix ARCH-$(ARCH)/, oskit_support.c globals.c init.c memmap.c)
SRC_C_x86-l4v2	= $(SRC_C_x86-l4x0)
SRC_S_x86-l4x0	= $(addprefix ARCH-$(ARCH)/, crt0.S)
SRC_S_x86-l4v2	= $(SRC_S_x86-l4x0)

CXXFLAGS	+= -fno-rtti

include $(L4DIR)/mk/prog.mk
