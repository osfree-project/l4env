# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		config.c tcb_generic.c tcb_l4.c thread.c memory.c stacks.c \
		create.c exit.c prio_generic.c prio_l4.c sleep.c data.c \
		lock.c l4.c
PRIVATE_INCDIR = ../../include
DEFINES = 	-DTHREAD_L4

# targets
TARGET =	libthread.a 
SYSTEMS =	x86-l4v2 x86-l4x0 arm-l4x0

# include lib role
include $(L4DIR)/mk/lib.mk
