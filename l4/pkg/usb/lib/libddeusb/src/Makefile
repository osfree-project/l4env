PKGDIR     ?= ../../../
L4DIR      ?= $(PKGDIR)/../..
CONTRIB_PATH ?= $(PKGDIR_ABS)/contrib
COMMON_PATH ?=  $(PKGDIR_ABS)/contrib


TARGET = libddeusb.a
CLIENTIDL = core.idl
SERVERIDL = gadget.idl


SYSTEMS = x86-l4v2

VPATH =  $(CONTRIB_PATH)


PRIVATE_INCDIR 	= $(PKGDIR_ABS)../dde/linux26/include $(MY_DDE_INCDIR) $(MY_LINUX26_INCDIR)  \
				  $(OBJ_BASE)/include/uclibc $(PKGDIR_ABS)/../usb/server/include


SRC_C                 =  libddeusb.c        



CFLAGS += 


include $(PKGDIR)/../dde/linux26/Makeconf

include $(L4DIR)/mk/lib.mk
