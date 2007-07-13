## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libgcc_c.a
libgcc_c_a_SOURCES=	parser.yy \
			scanner.ll 

AM_LFLAGS=         -Pgcc_c -d -o$(LEX_OUTPUT_ROOT).c
AM_YFLAGS=		-pgcc_c -d -v

CLEANFILES=             parser.cc $(LEX_OUTPUT_ROOT).cc \
			scanner.cc scanner.h y.output \
			parser.h parser.output

dist-hook:
	rm -f $(distdir)/parser.cc $(distdir)/$(LEX_OUTPUT_ROOT).cc \
	$(distdir)/scanner.cc $(distdir)/scanner.h $(distdir)/y.output

scanner.cc: parser.h

parser.h: parser.cc
