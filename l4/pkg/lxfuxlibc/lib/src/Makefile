PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libclxfux.o.a
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= lxfuxlc.c stdio.c syscall_check.c
CPPFLAGS	+= -nostdinc -ffunction-sections


include $(L4DIR)/mk/lib.mk
