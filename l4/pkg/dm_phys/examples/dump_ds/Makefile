PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= dump-ds
MODE		= host
SYSTEMS		= x86-l4v2 x86-l4x0

LIBCINCDIR	= $(addprefix -I,$(L4INCDIR))
LIBS		= -lnames -ll4util -lrmgr -ldm_mem

SRC_C		= main.c

include $(L4DIR)/mk/prog.mk
