PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

# the name of your library
TARGET	= libcpu_reserve.a

# list your .c files here
SRC_C	= clientlib.c delayed_preempt.c convenience.c startperiodic.c \
	  utcb_watch.c

# if your library implements the client side of an idl defined in an
# idl-file of your package, list the idl file name(s) here (no path needed)
CLIENTIDL = cpu_reserve.idl

include $(L4DIR)/mk/lib.mk
