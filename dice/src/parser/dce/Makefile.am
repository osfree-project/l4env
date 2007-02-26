## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libdce.a
libdce_a_SOURCES=	parser.yy \
			scanner.ll 

LFLAGS=         -Pdce -d -o$(LEX_OUTPUT_ROOT).c
YFLAGS=		-pdce -d -v

CLEANFILES=             parser.cc $(LEX_OUTPUT_ROOT).cc \
			scanner.cc scanner.h y.output

dist-hook:
	rm -f $(distdir)/parser.cc $(distdir)/$(LEX_OUTPUT_ROOT).cc \
	$(distdir)/scanner.cc $(distdir)/scanner.h $(distdir)/y.output

scanner.cc: parser.h

parser.h: parser.cc
