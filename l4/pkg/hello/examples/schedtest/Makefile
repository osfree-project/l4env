PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= schedtest
DEFAULT_RELOC	= 0x01000000
SYSTEMS		= x86-l4v2
SRC_C		= schedtest.c

# if you intend to build it with l4env, deactivate 
# MODE and L4LIBS and activate CFLAGS
MODE		= tiny
#CFLAGS		= -DUSE_L4ENV

include $(L4DIR)/mk/prog.mk
