PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libloglinux.a
MODE		= host
SYSTEMS		= x86

PRIVATE_INCDIR	= $(L4INCDIR) $(PKGDIR)/server/include $(PKGDIR)/lib/include
SRC_C		= log.c logll.c logL.c log_init.c log_printf.c \
		  logliblinux.c doprnt.c
VPATH		= $(PKGDIR)/lib/src

include $(L4DIR)/mk/lib.mk
