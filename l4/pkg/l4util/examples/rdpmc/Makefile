PKGDIR 		?= ../..
L4DIR 		?= $(PKGDIR)/../..

# source files
SRC_C 		= main.c

# target
TARGET		= rdpmc
SYSTEMS		= x86-l4v2
MODE		= l4linux
LIBS		= -lparsecmdline
L4LIBS		= $(ROOTLIB) -ll4sys

include $(L4DIR)/mk/prog.mk

LIBCLIBS_x86_l4linux =
