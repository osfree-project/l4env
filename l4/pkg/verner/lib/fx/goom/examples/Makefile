PKGDIR        ?= ../../../..
L4DIR         ?= $(PKGDIR)/../..

TARGET         = goom

DEFAULT_RELOC  = 0x00400000

PRIVATE_INCDIR = ../../ $(SDL_INCDIR)

SRC_C          = main.c
	
LIBS           += -lfxgoom -lSDL -ll4dope -lvscreen -ldsound -lcr7libm 

include $(L4DIR)/mk/prog.mk

