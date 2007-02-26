SYSTEMS			:= 	x86-l4v2
PKGDIR			?=	../../..
L4DIR			?=	$(PKGDIR)/../..

TARGET			=	libmpg123.a

PRIVATE_INCDIR		=	$(PKGDIR)/lib/include

SRC_C			=	common.c  dct64_i386.c  decode_i386.c  \
				interface.c  layer1.c  layer2.c  layer3.c  \
				mp3_common.c  tabinit.c

# include configuration
-include $(PKGDIR_OBJ)/Makeconf.bid.local

# use petze ?
ifeq ($(MPG123LIB_PETZE),y)
include $(L4DIR)/pkg/petze/mk/petze.mk
endif

# lib
include $(L4DIR)/mk/lib.mk

