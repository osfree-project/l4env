PKGDIR ?= ../../..
L4DIR  ?= $(PKGDIR)/../..

SYSTEMS = x86-l4v2

include $(PKGDIR)/../dde/linux26/Makeconf

TARGET = libore26.o.a

PRIVATE_INCDIR += $(PKGDIR)/server/lib-linux26/include \
		  $(LINUX26_INCDIR)

DEFINES = -D__KERNEL__ -DDDE_LINUX
WARNINGS = -Wall -Wstrict-prototypes -Wno-unused

CFLAGS = -ffunction-sections

SRC_C = ore.c

include $(L4DIR)/mk/lib.mk
