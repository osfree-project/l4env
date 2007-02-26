## Process this file with automake to produce Makefile.in

DOXYGEN=	@DOXYGEN@
RM_F=		rm -f

DOXFILES=	mainpage.dox backend.dox frontend.dox
EXTRA_DIST=	$(DOXFILES)
SUFFIXES=	.dox
CLEANFILES=	version.dox

if HAS_DOXYGEN
manual_HTML=	html/

all-local:	version.dox $(manual_HTML)

$(manual_HTML):	$(DOXFILES) Doxyfile
	$(DOXYGEN)

else
all-local:
	@echo *********************************************************
	@echo I cannot build the reference manual pages for dice, 
	@echo because doxygen could not be found during configure. 
	@echo To build the manual run 'doxygen' in the doc/html 
	@echo directory manually, or install doxygen and run configure 
	@echo again.
	@echo *********************************************************

endif

version.dox:
	@echo PROJECT_NUMBER = \"Version ${VERSION}\" > $@

clean-local:
	@rm -rf html/
