PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
TARGET		= kill-l4
SRC_C 		= main.c
MODE		= l4linux
LIBS		= -lgeneric_ts -llogserver_capsule -ll4sys
LDFLAGS		= -static
SYSTEMS		= x86-l4v2 arm-l4x0

include $(L4DIR)/mk/prog.mk
