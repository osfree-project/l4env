# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		main.c

# target
TARGET =	l4lload
SYSTEMS = 	x86-l4v2 x86-l4x0
MODE = 		l4linux
L4LIBS = 	

include $(L4DIR)/mk/prog.mk

# we do not need other L4 libs than l4lthread and l4util
LIBCLIBS_x86_l4linux = -lrmgr
