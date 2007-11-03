PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= uclibc_thread_safe
MODE		= l4env_minimal
SYSTEMS		= x86-l4v2
SRC_C		= main.c
DEFAULT_RELOC	= 0x00A00000
DEFINES		= -DL4_THREAD_SAFE
WANT_THREADSAFE	= y

include $(L4DIR)/mk/prog.mk
