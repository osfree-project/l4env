## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libidl.a

AM_LFLAGS=         -Pidl -d -o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=		-pidl -d -v
AM_CPPFLAGS=           -I$(top_builddir)/src
libidl_a_SOURCES=	idl-parser.tab.cc \
			idl-scanner.ll \
			idl-parser-driver.cc
noinst_HEADERS=		idl-parser-driver.hh

CLEANFILES=             idl-parser.tab.cc idl-parser.tab.hh idl-parser.output \
			idl-scanner.cc idl-scanner.h \
			location.hh position.hh stack.hh
# Because we do not list idl-parser.yy in the sources to build it "Our Own
# Way" (tm), we have to include it in the distribution using EXTRA_DIST.
EXTRA_DIST=		idl-parser.yy

dist-hook:
	rm -f $(addprefix $(distdir)/,$(CLEANFILES))

idl-parser.tab.cc: idl-parser.yy
	bison --report=all -d $<

idl-parser.tab.hh: idl-parser.tab.cc

idl-parser-driver.cc idl-parser-driver.hh idl-scanner.ll: idl-parser.tab.hh
