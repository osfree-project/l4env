## Process this file with automake to produce Makefile.in

SUBDIRS=	src include doc

DISTCLEANFILES= config.log config.status 
MAINTAINERCLEANFILES= aclocal.m4 depcomp mkinstalldirs stamp-h stamp-h1 install-sh missing config.h.in configure Makefile.in compile $(PACKAGE)-$(VERSION).tar.gz

all-local: 
	@cd include; make install-data-local; cd ..
