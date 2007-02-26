PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcontxt.a
MODE		= l4env
CLIENTIDL	= con.idl
SYSTEMS		= x86-l4v2 x86-l4x0

PRIVATE_INCDIR	= ../../include
vpath %.c	  ../../src
SRC_C		= printf.c contxt.c contxt_write.c contxt_read.c doprnt.c \
		  evh.c getchar.c intern.c putchar.c putstocon.c common.c \
		  puts.c

include $(L4DIR)/mk/lib.mk
