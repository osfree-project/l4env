PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= simple-exit
DEFAULT_RELOC	= 0x01300000
SYSTEMS		= x86-l4v2 x86-l4x0
SRC_C		= main.c
MODE		= $(if $(filter y,$(BUILD_LOADER)),loader,l4env)
DEFINES		= -DUSERNAME=\"$(shell whoami)\"
LIBS		= -lcontxt -lcon -lconstream-server -lloaderif \
		  -ldm_phys -levents -ll4util

include $(L4DIR)/mk/prog.mk
