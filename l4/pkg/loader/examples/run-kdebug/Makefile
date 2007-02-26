PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= run-kdebug
DEFAULT_RELOC	= 0x01300000
SYSTEMS		= x86-l4v2
SRC_C		= main.c direct.c direct_ihb.c key.c
MODE		= l4env
vpath main.c $(PKGDIR)/examples/run
DEFINES_main.c	= -DUSERNAME=\"$(shell whoami)\" \
		  -DDIRECT -I$(PKGDIR)/examples/run-kdebug
LIBS		= -lloaderif -ldm_phys -lparsecmdline -levents

include $(L4DIR)/mk/prog.mk

#ifneq ($(USE_DIETLIBC),)
#else
SRC_C		+= doprnt.c
#endif
