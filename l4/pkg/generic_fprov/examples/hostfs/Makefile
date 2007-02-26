PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

TARGET		= hostfs
SRC_C		= fs.c main.c comm.c
MODE		= l4env_freebsd
LIBS		= -lparsecmdline -lgeneric_fprov-server
DEFAULT_RELOC   = 0x01210000

include $(L4DIR)/mk/prog.mk
