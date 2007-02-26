SYSTEMS			:= 	x86-l4v2
PKGDIR			?=	../../..
L4DIR			?=	$(PKGDIR)/../..

PRIVATE_INCDIR		 = 	$(PKGDIR)/include $(PKGDIR)/lib/include $(PKGDIR_OBJ)/lib/include

TARGET			 =	libavilib.a

SRC_C			 =	avilib.c

# include configuration
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf.bid.local

# use petze ?
ifeq ($(AVILIB_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif

# lib
include $(L4DIR)/mk/lib.mk

