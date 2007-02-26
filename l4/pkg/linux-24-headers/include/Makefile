#
# $Id$
#
# Makefile for the Linux 2.4 header include package
#

PKGDIR	= ..
L4DIR	?= $(PKGDIR)/../..

INSTALL_INC_PREFIX = linux-24

include $(L4DIR)/mk/include.mk

# macro to establish a symlink to a dir if not already existing
MAKELINK = if [ "`readlink $2`" != "asm-i386" ] ; then ln -sf "$1" "$2" ; fi

all::
	$(VERBOSE)$(call MAKELINK,asm-i386,$(INSTALLDIR_LOCAL)/$(INSTALL_INC_PREFIX)/asm)

install::
	$(VERBOSE)$(call MAKELINK,asm-i386,$(INSTALLDIR)/$(INSTALL_INC_PREFIX)/asm)

