PKGDIR ?= ../..
L4DIR  ?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

# depends on oskit, only build if available
ifeq ($(USE_DIETLIBC)$(USE_UCLIBC),nn)
TARGET		= hostfs
MODE		= l4env_freebsd
endif

SRC_C		= fs.c main.c comm.c
LIBS		= -lparsecmdline -lgeneric_fprov-server
DEFAULT_RELOC   = 0x01210000

include $(L4DIR)/mk/prog.mk
