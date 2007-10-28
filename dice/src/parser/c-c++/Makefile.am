## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libcparser.a

AM_LFLAGS=         -Pc -d -o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=		-pc -d -v
AM_CPPFLAGS=           -I$(top_builddir)/src
libcparser_a_SOURCES=	c-parser.tab.cc \
			c-scanner.ll \
			c-parser-driver.cc
noinst_HEADERS=		c-parser-driver.hh

CLEANFILES=             c-parser.tab.cc c-parser.tab.hh c-parser.output \
			c-scanner.cc c-scanner.h \
			location.hh position.hh stack.hh
# Because we do not list c-parser.yy in the sources to build it "Our Own
# Way" (tm), we have to include it in the distribution using EXTRA_DIST.
EXTRA_DIST=		c-parser.yy

dist-hook:
	rm -f $(addprefix $(distdir)/,$(CLEANFILES))

c-parser.tab.cc: c-parser.yy
	$(YACC) --report=all -d $<

c-parser.tab.hh: c-parser.tab.cc

c-parser-driver.cc c-parser-driver.hh c-scanner.ll: c-parser.tab.hh
