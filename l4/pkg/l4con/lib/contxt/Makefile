PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libcontxt.a
MODE		= l4env
CLIENTIDL	= l4con.idl
SYSTEMS		= x86-l4v2 arm-l4v2

PRIVATE_INCDIR	= $(SRC_DIR)/../include
vpath %.c	  $(SRC_DIR)/../src
SRC_C		= printf.c contxt.c contxt_write.c contxt_read.c doprnt.c \
		  evh.c getchar.c intern.c putchar.c putstocon.c common.c \
		  puts.c

include $(L4DIR)/mk/lib.mk
