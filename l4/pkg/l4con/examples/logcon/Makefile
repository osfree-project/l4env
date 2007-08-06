PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SUBDIRS		= idl dumplog

SYSTEMS		= x86-l4v2 amd64-l4v2 arm-l4v2
TARGET		= logcon
LIBS		= -u putchar -u puts -lcontxt -lcon -lconstream-server
DEFAULT_RELOC	= 0x00300000
SERVERIDL       = log.idl logcon.idl
IDL_PKGDIR	= $(PKGDIR_OBJ)/../log \
		  $(PKGDIR_OBJ)/examples/logcon

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk

$(foreach arch,$(TARGET_SYSTEMS), $(OBJ_DIR)/OBJ-$(arch)): idl

all:: $(SUBDIRS)

dumplog: idl
