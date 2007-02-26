## Process this file with automake to produce Makefile.in

DOXYGEN=	@DOXYGEN@
RM_F=		rm -f

DOXFILES=	mainpage.dox backend.dox frontend.dox
EXTRA_DIST=	$(DOXFILES)
SUFFIXES=	.dox
CLEANFILES=	dox-stamp version.dox

if HAS_DOXYGEN
manual_HTML=	html/

all-local:	dox-stamp version.dox $(manual_HTML)

$(manual_HTML):	$(DOXFILES) Doxyfile
	$(DOXYGEN)

endif

version.dox:
	echo \\newcommand{\\Version}{${VERSION}} > $@

dox-stamp: $(DOXFILES)
	@for F in $(DOXFILES); do \
	  if test ! -f $${F} -a ! -L $${F}; then \
	    ${LN_S} $(srcdir)/$${F} $${F}; \
	  fi \
	done
	@echo timestamp > dox-stamp 2>/dev/null

clean-local:
	rm -rf html/
	for F in $(DOXFILES); do \
	  test ! -L $${F} || ${RM_F} $${F}; \
	done
