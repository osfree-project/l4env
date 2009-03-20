PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..
TARGET  = fprov-l4
SRC_C   = main.c l4env_dummies.c
MODE    = l4linux
LIBS    = -ldm_generic -lfprov_ext-server -lz \
          -lparsecmdline 
LDFLAGS = -static

include $(L4DIR)/mk/prog.mk
