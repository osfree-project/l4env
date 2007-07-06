PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= simple-exit
DEFAULT_RELOC	= 0x01300000
SYSTEMS		= x86-l4v2 arm-l4v2
SRC_C		= main.c
MODE		= loader
LIBS		= -lcontxt -lcon -lconstream-server -lloaderif \
		  -ldm_phys -levents -ll4util

DEPENDS_PKGS    = l4con

include $(L4DIR)/mk/prog.mk
