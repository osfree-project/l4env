PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libtask_server.o.a
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

SERVERIDL = generic_ts.idl

SRC_C = server.c

BUILD_PIC += libtask_server.o.a

include $(L4DIR)/mk/lib.mk
