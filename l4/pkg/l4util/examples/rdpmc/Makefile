PKGDIR 		?= ../..
L4DIR 		?= $(PKGDIR)/../..

# source files
SRC_C 		= main.c

# target
TARGET		= rdpmc
SYSTEMS		= x86-l4v2 x86-l4x0
MODE		= l4linux
L4LIBS		=

include $(L4DIR)/mk/prog.mk

LIBCLIBS_x86_l4linux = $(ROOTLIB) -lparsecmdline
