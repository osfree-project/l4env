PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= many_tasks
MODE		= tiny
DEFAULT_RELOC	= 0x01000000
SYSTEMS		= x86-l4v2 amd64-l4v2
SRC_C		= many_tasks.c

include $(L4DIR)/mk/prog.mk

L4LIBS		+= $(ROOTLIB)
