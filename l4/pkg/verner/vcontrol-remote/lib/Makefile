PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../../..

include $(L4DIR)/mk/Makeconf

ifeq "$(BUILD_LOADER)" "y"
TARGET		= libvcontrol-remote.s.so
INSTALL_TARGET	= libvcontrol-remote.so
else
TARGET		=
INSTALL_TARGET	=
endif

MODE	= l4env

CLIENTIDL = vcontrol-remote.idl

LIBS    = --whole-archive \
	  -lnames.p       \
	  -ll4util.p      \
	  $(ROOTPLIB)

include $(L4DIR)/mk/lib.mk

$(INSTALL_TARGET): $(TARGET)
	$(VERBOSE)$(CP) $^ $@
