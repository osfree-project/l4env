## Process this file with automake to produce Makefile.in

DOXYGEN=	@DOXYGEN@
RM_F=		rm -f

DOXFILES=	manpage.dox
EXTRA_DIST=	$(DOXFILES) Doxyfile
SUFFIXES=	.dox
CLEANFILES=	version.dox

if HAS_DOXYGEN
man_MANS=	dice.1

all-local:	$(man_MANS)

# because man_MANS is mapped to MAN
# and MAN is before all-local in the
# dependency list of all, we have to
# add version.dox here
$(man_MANS):	version.dox $(DOXFILES) Doxyfile
	$(VERBOSE)DOX_SRCDIR=$(srcdir) $(DOXYGEN) $(srcdir)/Doxyfile
	$(VERBOSE)$(LN_S) -f man/man1/dice.1

else
all-local:
	@echo *********************************************************
	@echo I cannot build the manual page for dice, because doxygen
	@echo could not be found during configure. To build the manual
	@echo run 'doxygen' in the doc/manual directory manually, or
	@echo install doxygen and run configure again.
	@echo *********************************************************

endif

version.dox:
	echo PROJECT_NUMBER = \"Version ${VERSION}\" > $@

clean-local:
	rm -rf man/ man1/
	rm -f dice.1

