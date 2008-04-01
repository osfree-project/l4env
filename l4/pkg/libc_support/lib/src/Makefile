PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

TARGET		= libuclibc_support.a

MODE		= sigma0
SYSTEMS		= x86-l4v2 arm-l4v2 amd64-l4v2

SRC_C_libuclibc_support.a   = console.c panic.c getchar.c read.c
SRC_C_libuclibc_support.p.a = $(SRC_C_libuclibc_support.a)

include $(L4DIR)/mk/lib.mk
