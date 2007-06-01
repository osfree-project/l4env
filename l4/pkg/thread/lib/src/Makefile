# directories we need to know
PKGDIR ?=	../..
L4DIR ?=	$(PKGDIR)/../..

# source files
SRC_C =		config.c tcb_generic.c tcb_l4.c thread.c memory.c stacks.c \
		create.c exit.c prio_generic.c prio_l4.c sleep.c data.c \
		lock.c l4.c
SRC_C_amd64-l4v2 :=  ARCH-amd64/thread_setup.c
SRC_C_x86-l4v2 := thread_setup_dummy.c
SRC_C_arm-l4v2 := thread_setup_dummy.c
PRIVATE_INCDIR = $(SRC_DIR)/../include
DEFINES = 	-DTHREAD_L4

# targets
TARGET =	libthread.a 
SYSTEMS =	x86-l4v2 arm-l4v2 amd64-l4v2

# include lib role
include $(L4DIR)/mk/lib.mk
