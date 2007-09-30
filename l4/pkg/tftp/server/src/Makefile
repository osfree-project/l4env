PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2
DEFAULT_RELOC	= 0x01000000
SUBDIRS		= netboot
TARGET		= tftp

SRC_C		= main.c
LIBS		= -lgeneric_fprov-server \
		  -L../netboot/OBJ-$(SYSTEM) -lnetboot -lore

PRIVATE_INCDIR	= $(OBJ_DIR)/netboot

DEFINES		= -DWITHOUT_LIBC_STUBS

all:: $(SUBDIRS)

include $(L4DIR)/mk/prog.mk

$(foreach arch,$(TARGET_SYSTEMS), $(OBJ_DIR)/OBJ-$(arch)): netboot
