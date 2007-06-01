PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

TARGET		= simple_ts
MODE		= sigma0
DEFAULT_RELOC   = 0x01400000
DEFAULT_RELOC_arm = 0x001f0000
PRIVATE_INCDIR  = .

SRC_C		= main.c
LIBS		= -lgeneric_ts-server -levents -lgeneric_ts \
		  -lparsecmdline -ll4env_err

PRIVATE_INCDIR	= $(PKGDIR)/server/src

include $(L4DIR)/mk/prog.mk
