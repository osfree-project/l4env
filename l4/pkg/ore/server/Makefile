PKGDIR	?= ..
L4DIR	?= $(PKGDIR)/../..

# the default is to build the listed directories, provided that they
# contain a Makefile. If you need to change this, uncomment the following
# line and adapt it.
TARGET = idl lib lib26 src

DROPSCONF	= y
DROPSCONF_TITLE = ORe configuration
DROPSCONF_UNDEF = 1
DROPSCONF_CONFIG_H = $(OBJ_DIR)/include/auto_config.h
DROPSCONF_DEFCONFIG = defconfig

include $(L4DIR)/mk/subdir.mk

# these dependencies are defined in subdir.mk only if PKGDIR=.
lib: idl
src: lib
