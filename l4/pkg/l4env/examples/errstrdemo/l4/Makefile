PKGDIR		?= ../../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= l4env_err_l4ipc
PRIVATE_INCDIR	= $(L4INCDIR)

SRC_C		= main.c
DEFAULT_RELOC	= 0x00400000
vpath main.c $(PKGDIR)/examples/errstrdemo/linux

include $(L4DIR)/mk/prog.mk
