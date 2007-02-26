PKGDIR	?= ../..
L4DIR	?= $(PKGDIR)/../..

SRC_C		= main.c
DEFAULT_RELOC	= 0x009f0000
LIBS		= -lparsecmdline -lcpu_reserve -lrt_mon_client
TARGET		= utcbwatch
PRIVATE_INCDIR	= $(PKGDIR)/lib/src

include $(L4DIR)/mk/prog.mk
