PKGDIR?= ../..
L4DIR ?= $(PKGDIR)/../..

SYSTEMS   := x86-l4v2 x86-l4x0
TARGET	  = liblyon-client.a
BUILD_PIC = $(TARGET)
SRC_C	  = encap.c
CLIENTIDL = lyon.idl
WARNINGS  = -Wall -Wstrict-prototypes

include $(L4DIR)/mk/lib.mk
