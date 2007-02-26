## Process this file with automake to produce Makefile.in

SUBDIRS=	src include

DISTCLEANFILES= config.log config.status 
MAINTAINERCLEANFILES= aclocal.m4 depcomp mkinstalldirs stamp-h stamp-h1 install-sh missing config.h.in configure Makefile.in compile $(PACKAGE)-$(VERSION).tar.gz

all-local: 
	@cd include; $(MAKE) install-data-local; cd ..

doc:
	@cd doc; $(MAKE); cd ..

install-data-local:
	@cd doc/manual; $(MAKE) install; cd ../..

test:
	$(MAKE) -C examples test

.PHONY: doc
