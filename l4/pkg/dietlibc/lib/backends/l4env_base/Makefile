PKGDIR	?= ../../..
L4DIR	?= $(PKGDIR)/../..

SYSTEMS	= x86-l4v2 x86-l4x0
MODE	= l4env_base
TARGET	= libdiet_be_l4env_base.o.a libdiet_memory-debug.o.a
SRC_C_libdiet_be_l4env_base.o.a = startup.c fops.c buddy.c log-output.c \
				malloc.c _exit.c abort.c
SRC_C_libdiet_memory-debug.o.a  = debug-buddy.c

all::
debug-%.c:%.c .general.d
	$(VERBOSE)echo '#include "../$*.c"' >$@
debug-buddy.o: DEFINES	= -DDEBUG_BUDDY=1

include $(L4DIR)/mk/lib.mk
