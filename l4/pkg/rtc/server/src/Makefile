PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..
SYSTEMS		= x86-l4v2
TARGET_x86	= rtc rtc-ux
TARGET_amd64	= rtc
TARGET		= $(TARGET_$(ARCH))
MODE		= sigma0
DEFAULT_RELOC	= 0x01080000
SYSTEMS		= x86-l4v2 amd64-l4v2

SRC_C_rtc	= main.c x86.c
SRC_C_rtc-ux	= main.c ux.c
SERVERIDL	= rtc.idl

LIBS_rtc-ux	= -lclxfux.o

include $(L4DIR)/mk/prog.mk
