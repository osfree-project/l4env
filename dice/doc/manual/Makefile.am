## Process this file with automake to produce Makefile.in

DOXYGEN=	@DOXYGEN@
RM_F=		rm -f

DOXFILES=	manpage.dox
EXTRA_DIST=	$(DOXFILES)
SUFFIXES=	.dox
CLEANFILES=	dox-stamp version.dox

if HAS_DOXYGEN
man_MANS=	dice.1

all-local:	dox-stamp $(man_MANS)

$(man_MANS):	$(DOXFILES) Doxyfile
	$(VERBOSE)$(DOXYGEN)
	$(VERBOSE)$(LN_S) -f man/man1/dice.1

endif

dox-stamp: $(DOXFILES)
	@for F in $(DOXFILES); do \
	  if test ! -f $${F} -a ! -L $${F}; then \
	    ${LN_S} $(srcdir)/$${F} $${F}; \
	  fi \
	done
	@echo timestamp > dox-stamp 2>/dev/null

clean-local:
	rm -rf man/
	rm -f dice.1
	for F in $(DOXFILES); do \
	  test ! -L $${F} || ${RM_F} $${F}; \
	done
