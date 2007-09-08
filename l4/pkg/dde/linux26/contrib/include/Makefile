PKGDIR	?= ../../..
L4DIR	?= $(PKGDIR)/../..

# Force these include files to appear in a special subfolder of dde/
INSTALL_INC_PREFIX = l4/dde/linux26/linux-headers

include $(L4DIR)/mk/include.mk

SUFFIX_amd64 = x86_64
SUFFIX_x86   = i386

# macro to establish a symlink to a dir if not already existing
MAKELINK = if [ ! $1 -ef $2 ] ; then ln -sf $3 $2 ; fi

ARCH = $(BUILD_ARCH)

all::
	$(VERBOSE)$(MKDIR) $(INSTALLDIR_LOCAL)/$(ARCH)/$(INSTALL_INC_PREFIX)
	$(VERBOSE)$(call MAKELINK, \
		$(INSTALLDIR_LOCAL)/$(INSTALL_INC_PREFIX)/asm-$(SUFFIX_$(ARCH)), \
		$(INSTALLDIR_LOCAL)/$(ARCH)/$(INSTALL_INC_PREFIX)/asm,           \
		$(INSTALLDIR_LOCAL)/$(INSTALL_INC_PREFIX)/asm-$(SUFFIX_$(ARCH)))

install::
	$(VERBOSE)$(MKDIR) $(INSTALLDIR)/$(ARCH)/$(INSTALL_INC_PREFIX)
	$(VERBOSE)$(call MAKELINK, \
		$(INSTALLDIR)/$(ARCH)/$(INSTALL_INC_PREFIX)/asm-$(SUFFIX_$(ARCH)), \
		$(INSTALLDIR)/$(ARCH)/$(INSTALL_INC_PREFIX)/asm                    \
		$(INSTALLDIR)/$(ARCH)/$(INSTALL_INC_PREFIX)/asm-$(SUFFIX_$(ARCH)))

