PKGDIR		?= ../..
L4DIR		?= $(PKGDIR)/../..

SUBDIRS		= netboot
SYSTEMS		= x86-l4v2 x86-l4x0

all:: $(SUBDIRS)

DROPSCONF	= y
DROPSCONF_CONFIG = Makeconf
DROPSCONF_CONFIG_H = tftp_config.h
include $(L4DIR)/mk/config.inc

include $(L4DIR)/mk/prog.mk
