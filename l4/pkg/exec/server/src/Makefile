PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 x86-l4x0
TARGET		= l4exec
DEFAULT_RELOC_x86 = 0x01100000
DEFAULT_RELOC_arm = 0x001e0000
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

PRIVATE_INCDIR	= ../../include
LIBS		= -lgeneric_fprov -ldm_phys -ll4exec -ldemangle

SRC_CC		= elf.cc elf32.cc elf32_dbg.cc elf64.cc \
		  bin_obj.cc exc_obj.cc exc_obj_psec.cc exc_obj_stab.cc \
		  exc.cc exc_img.cc dsc.cc file.cc check.cc main.cc
SERVERIDL	= exec.idl
CXXFLAGS	= -fno-rtti -fno-exceptions

include $(L4DIR)/mk/prog.mk

