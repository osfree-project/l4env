PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

MODE		= libc
LIBCLIBS	= 
#LIBCINCDIR	= 
SRC_C          := lcd.c lcd-virtual.c lcd-h3800.c
SYSTEMS		= arm-l4x0
TARGET		= libarm_lcd.o.a

PRIVATE_INCDIR	+= $(PKGDIR)/lcd/src

-include $(SRC_DIR)/isg/Makefile.include

include $(L4DIR)/mk/lib.mk

