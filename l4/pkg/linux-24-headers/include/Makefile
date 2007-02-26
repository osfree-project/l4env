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
MAKELINK = if [ ! $1 -ef $2 ] ; then ln -sf $3 $2 ; fi

all::
	$(VERBOSE)$(call MAKELINK, \
		$(INSTALLDIR_LOCAL)/$(INSTALL_INC_PREFIX)/asm-i386,\
		$(INSTALLDIR_LOCAL)/$(INSTALL_INC_PREFIX)/asm, asm-i386)

install::
	$(VERBOSE)$(call MAKELINK, \
		$(INSTALLDIR)/$(INSTALL_INC_PREFIX)/asm-i386,\
		$(INSTALLDIR)/$(INSTALL_INC_PREFIX)/asm, asm-i386)

