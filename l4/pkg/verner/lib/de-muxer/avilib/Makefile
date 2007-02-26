SYSTEMS			:= 	x86-l4v2
PKGDIR			?=	../../..
L4DIR			?=	$(PKGDIR)/../..

PRIVATE_INCDIR		 = 	$(PKGDIR)/lib/include $(PKGDIR)/include

TARGET			 =	libavilib.a

SRC_C			 =	avilib.c

# include configuration
-include $(PKGDIR)/Makeconf.bid.local

# use petze ?
ifeq ($(AVILIB_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif

# lib
include $(L4DIR)/mk/lib.mk

