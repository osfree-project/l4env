PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2

TARGET		= lib$(PKGNAME)26.o.a

# Most paths & co. come from DDE's Makeconf file
include $(PKGDIR)/../dde/linux26/Makeconf

SRC_C	= socket.c dummy.c

PRIVATE_INCDIR	+= $(PKGDIR)/lib/include
# $(DDE_INCDIR) $(LINUX24_INCDIR)
DEFINES = -D__KERNEL__ -DDDE_LINUX
WARNINGS	= -Wall -Wstrict-prototypes

CFLAGS = -ffunction-sections

include $(L4DIR)/mk/Makeconf
ifeq ($(LINUX26_DIR),)
  $(error library needs Linux-2.6 sources! Set LINUX26_DIR to Linux 2.6.20.19 source tree to change that.)
endif

vpath %.c $(LINUX26_DIR)
vpath %.S $(LINUX26_DIR)

NET_C	= $(addprefix net/core/, datagram.c iovec.c scm.c skbuff.c sock.c)

LIB_C += lib/ctype.c
LIB_S += arch/i386/lib/checksum.S

### Now put it all together
SRC_C += $(NET_C) $(LIB_C)
SRC_S += $(LIB_S)

include $(L4DIR)/mk/lib.mk
