PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= liblog.a liblogserver.a liblogserver_capsule.a
MODE		= sigma0
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

SRC_C		= log_init.c log.c logll.c logL.c lock.c \
		  doprnt.c log_printf.c fiasco_tbuf.c
SRC_C_liblog.a	= loglib.c printf.c sprintf.c
SRC_C_liblog.pr.a	= loglib.c printf.c sprintf.c
SRC_C_liblogserver.a = logserver.c logchannel.c printf.c sprintf.c
SRC_C_liblogserver.p.a = logserver.c logchannel.c printf.c sprintf.c
SRC_C_liblogserver.pr.a = logserver.c logchannel.c printf.c sprintf.c
SRC_C_liblogserver_capsule.a = logserver.c logchannel.c
SRC_C_liblogserver_capsule.p.a = logserver.c logchannel.c
SRC_C_liblogserver_capsule.pr.a = logserver.c logchannel.c

CPPFLAGS	= -D__USE_L4WQLOCKS__ -D__L4__
OPTS		= -g -Os $(CARCHFLAGS_$(ARCH)) $(CARCHFLAGS_$(ARCH)_$(CPU))
PRIVATE_INCDIR	= $(PKGDIR)/server/include $(PKGDIR)/lib/include
VPATH		= $(PKGDIR)/lib/src
CLIENTIDL_liblogserver.a	= log.idl
CLIENTIDL_liblogserver.p.a	= log.idl
CLIENTIDL_liblogserver.pr.a	= log.idl
CLIENTIDL_liblogserver_capsule.a = log.idl
CLIENTIDL_liblogserver_capsule.p.a = log.idl
CLIENTIDL_liblogserver_capsule.pr.a = log.idl

include $(L4DIR)/mk/lib.mk
