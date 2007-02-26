PKGDIR  ?= ../..
L4DIR   ?= $(PKGDIR)/../..

SYSTEMS  = x86-l4v2

TARGET   = libc_be_l4sig.o.a

SRC_C    = startup.c kill.c procmask.c enqueue.c dispatch.c  \
	   sigaction.c default_handling.c signal.c alarm.c \
	   timedwait.c exceptions.c

SRC_S    = except.S

CFLAGS   = -ffunction-sections
MODE     = l4env_minimal

include $(L4DIR)/mk/lib.mk
