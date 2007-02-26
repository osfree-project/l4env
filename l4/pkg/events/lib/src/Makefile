PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

TARGET		= libevents.a
SYSTEMS		= x86-l4v2 arm-l4x0 amd64-l4v2
MODE		= sigma0
PRIVATE_INCDIR	= $(PKGDIR)/server/include
SRC_C		= libevents.c message.c init.c wait.c
CFLAGS		= -ffunction-sections

include $(L4DIR)/mk/lib.mk
