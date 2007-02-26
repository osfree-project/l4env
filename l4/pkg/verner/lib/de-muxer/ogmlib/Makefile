SYSTEMS			:= 	x86-l4v2
PKGDIR			?=	../../..
L4DIR			?=	$(PKGDIR)/../..

PRIVATE_INCDIR		 = 	$(PKGDIR)/include $(PKGDIR)/lib/include $(PKGDIR_OBJ)/lib/include

TARGET			 =	libogmlib.a

SRC_C			 =	ogmlib.c framing.c bitwise.c common.c

# include configuration
include $(L4DIR)/mk/Makeconf
-include $(PKGDIR_OBJ)/Makeconf.bid.local

# use petze ?
ifeq ($(OGMLIB_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif

# lib
include $(L4DIR)/mk/lib.mk

