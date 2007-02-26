PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= list-ds
MODE		= host
SYSTEMS		= x86-l4v2 x86-l4x0

LIBCINCDIR	= $(addprefix -I,$(L4INCDIR))
LIBS		= -lnames -llogserver -ll4util $(ROOTLIB) -ldm_generic

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
