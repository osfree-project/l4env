PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SUBDIRS		= netboot
SYSTEMS		= x86-l4v2

all:: $(SUBDIRS)

DROPSCONF	= y
DROPSCONF_UNDEF	= 1
DROPSCONF_CONFIG_MK = $(OBJ_DIR)/Makeconf
DROPSCONF_CONFIG_H = $(OBJ_DIR)/tftp_config.h

include $(L4DIR)/mk/prog.mk

$(foreach arch,$(TARGET_SYSTEMS), $(OBJ_DIR)/OBJ-$(arch)): netboot
