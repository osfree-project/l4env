PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(USE_UCLIBC),y)
TARGET		= uclibc_thread_unsafe
endif
MODE		= l4env_uclibc_minimal
SYSTEMS		= x86-l4v2
SRC_C		= main.c
DEFAULT_RELOC	= 0x00A00000
LIBS		= -luc_be_minimal_io

vpath %.c $(PKGDIR)/examples/thread_safe

include $(L4DIR)/mk/prog.mk
