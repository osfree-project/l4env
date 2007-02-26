PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(USE_UCLIBC),y)
TARGET		= uclibc_thread_safe
endif
MODE		= l4env_uclibc_minimal
SYSTEMS		= x86-l4v2 x86-l4x0
SRC_C		= main.c
DEFAULT_RELOC	= 0x00A00000
DEFINES		= -DL4_THREAD_SAFE
WANT_THREADSAFE	= y

include $(L4DIR)/mk/prog.mk
