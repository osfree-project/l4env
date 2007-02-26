PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 x86-l4x0
TARGET		= l4exec
DEFAULT_RELOC	= 0x01100000
SYSTEMS		= x86-l4v2 x86-l4x0

PRIVATE_INCDIR	= ../../include
LIBS		= -lgeneric_fprov -ldm_phys

SRC_CC		= elf.cc elf32.cc elf32_dbg.cc elf64.cc \
		  bin_obj.cc exc_obj.cc exc_obj_psec.cc exc_obj_stab.cc \
		  exc.cc exc_img.cc dsc.cc file.cc check.cc main.cc
SERVERIDL	= exec.idl
CXXFLAGS	= -fno-rtti -fno-exceptions
CPPFLAGS	= -DL4_MAX_RPC_BUFFER_SIZE=512

include $(L4DIR)/mk/prog.mk

