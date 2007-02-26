PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

include $(L4DIR)/mk/Makeconf

ifeq ($(USE_UCLIBC),y)
TARGET		= libuclibc_support.a
else
TARGET		= libdietlibc_support.a
endif

MODE		= sigma0
SYSTEMS		= x86-l4v2 x86-l4x0 arm-l4x0

SRC_C_libdietlibc_support.a = console.c panic.c getchar.c
SRC_C_libuclibc_support.a   = console.c panic.c getchar.c read.c
SRC_C_libuclibc_support.p.a = $(SRC_C_libuclibc_support.a)

include $(L4DIR)/mk/lib.mk
