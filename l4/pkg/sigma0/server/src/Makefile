SYSTEMS		:= x86-l4v2 arm-l4v2 amd64-l4v2
PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= sigma0

DEPENDS_PKGS	= cxx

LIBCLIBS	= -nostdlib $(LIBCLIBS_X86) $(GCCLIB)
L4LIBS		= -lcxx_io -lcxx_io_kdebug -lcxx_base -ll4sys
CRT0		=
MODE		= tiny
DEFAULT_RELOC_x86   := 0x00090000
DEFAULT_RELOC_amd64 := 0x00090000
DEFAULT_RELOC_arm   := 0x0004e000

vpath %.c $(L4DIR)/pkg/crtx/lib/src

PRIVATE_INCDIR	= $(SRC_DIR)

SRC_CC := init.cc memmap.cc page_alloc.cc mem_man.cc
SRC_CC_x86-l4v2   := init_mem_v2.cc ioports_x86.cc
SRC_CC_amd64-l4v2 := init_mem_v2.cc ioports_x86.cc
SRC_CC_arm-l4v2   := init_mem_arm.cc ioports_none.cc
SRC_C  := support.c construction.c
SRC_S  := $(addprefix ARCH-$(ARCH)/, crt0.S)

include $(L4DIR)/mk/prog.mk

L4LIBS		+= -lcxx_start -lcxx_io -lcxx_io_kdebug -lcxx_base 
CXXFLAGS	+= -fno-rtti -fno-exceptions
