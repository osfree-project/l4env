PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SYSTEMS		= x86-l4v2

TARGET		= lib$(PKGNAME).o.a

SRC_C	= socket.c dummy.c

PRIVATE_INCDIR	= $(PKGDIR)/lib/include $(DDE_INCDIR) $(LINUX24_INCDIR)
DEFINES = -D__KERNEL__ -DDDE_LINUX -DHZ=100
WARNINGS	= -Wall -Wstrict-prototypes

CFLAGS = -ffunction-sections

include $(L4DIR)/mk/Makeconf
ifeq ($(LINUX24_AVAIL),n)
  $(error library needs Linux-2.4 sources! Enable LINUX24_AVAIL via "make config".)
endif

vpath %.c $(LINUX24_DIR)
vpath %.S $(LINUX24_DIR)

NET_C	= $(addprefix net/core/, datagram.c iovec.c scm.c skbuff.c sock.c)

LIB_C += lib/ctype.c
LIB_S += arch/i386/lib/checksum.S

### Now put it all together
SRC_C += $(NET_C) $(LIB_C)
SRC_S += $(LIB_S)

include $(L4DIR)/mk/lib.mk
