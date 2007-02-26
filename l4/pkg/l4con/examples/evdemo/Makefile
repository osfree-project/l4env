PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= con_evdemo
LIBS		= -lcon -lconstream-server
DEFAULT_RELOC	= 0x00800000
SYSTEMS		= x86-l4v2

SRC_C		= evdemo.c
PRIVATE_INCDIR	= $(SRC_DIR)/../include

include $(L4DIR)/mk/prog.mk
