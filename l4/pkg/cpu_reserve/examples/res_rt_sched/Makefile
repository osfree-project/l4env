PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= res_rt_sched
DEFAULT_RELOC	= 0x01000000
SRC_C		= main.c watch.c
LIBS		= -lparsecmdline -lcpu_reserve

include $(L4DIR)/mk/prog.mk
