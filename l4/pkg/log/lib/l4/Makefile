PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= liblog.a liblogserver.a liblogserver_capsule.a
MODE		= oskit10_sigma0
SYSTEMS		= x86-l4v2 x86-l4x0

SRC_C		= log_init.c log.c logl.c logL.c lock.c \
		  doprnt.c log_printf.c
SRC_C_liblog.a	= loglib.c printf.c sprintf.c
SRC_C_liblogserver.a = logserver.c printf.c sprintf.c
SRC_C_liblogserver.p.a = logserver.c printf.c sprintf.c
SRC_C_liblogserver_capsule.a = logserver.c

CPPFLAGS	= -D__USE_L4WQLOCKS__ -D__L4__
PRIVATE_INCDIR	= $(PKGDIR)/server/include $(PKGDIR)/lib/include
VPATH		= $(PKGDIR)/lib/src

include $(L4DIR)/mk/lib.mk
