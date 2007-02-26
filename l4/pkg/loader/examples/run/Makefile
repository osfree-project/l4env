PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= run
DEFAULT_RELOC	= 0x01300000
SRC_C		= main.c
LIBS		= -lcontxt -lcon -lconstream-server -llogserver -lloaderif \
		  -ldm_phys

include $(L4DIR)/mk/prog.mk
