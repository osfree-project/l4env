# directories we need to know
PKGDIR =	.
L4DIR ?=	$(PKGDIR)/../..

# config-tool
DROPSCONF 		= y
DROPSCONF_CONFIG_IN	= $(PKGDIR)/config/verner_config.in
DROPSCONF_CONFIG_H	= $(PKGDIR_OBJ)/config/verner_config.h
DROPSCONF_HELPFILE	= $(PKGDIR)/config/verner_config.help
DROPSCONF_DEFCONFIG	= $(PKGDIR)/config/verner_config.default

SUBDIRS	= vdemuxer vcore vsync vcontrol vcontrol-remote

# include subdir role
include $(L4DIR)/mk/subdir.mk
