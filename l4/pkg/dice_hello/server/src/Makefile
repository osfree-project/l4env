PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dice_hello_server

SERVERIDL	= hello.idl
SRC_C		= main.c
DEFAULT_RELOC	= 0x00380000
MODE		= l4env_minimal

include	$(L4DIR)/mk/prog.mk
