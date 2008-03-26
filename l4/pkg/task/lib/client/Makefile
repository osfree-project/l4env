PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libgeneric_ts.a
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

CLIENTIDL	= generic_ts.idl

SRC_C = client.c wrap.c

BUILD_PIC += libgeneric_ts.a

include $(L4DIR)/mk/lib.mk
