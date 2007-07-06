PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= run
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2
DEFAULT_RELOC	= 0x01300000
SRC_C		= main.c
DEFINES		= -DUSERNAME=\"$(shell whoami)\"

DEPENDS_PKGS	= l4con

include $(L4DIR)/mk/Makeconf

ifeq ($(ARCH),amd64)
MODE		= l4env
else
MODE		= loader
endif

LIBS		= -lcontxt -lconstream-server -ldm_phys -lparsecmdline -levents -lloaderif

include $(L4DIR)/mk/prog.mk
