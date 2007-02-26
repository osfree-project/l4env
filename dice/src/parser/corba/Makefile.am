## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libcorba.a
libcorba_a_SOURCES=	parser.yy \
			scanner.ll

LFLAGS=         -Pcorba -d -o$(LEX_OUTPUT_ROOT).c
YFLAGS=		-pcorba -d -v

CLEANFILES=             parser.cc $(LEX_OUTPUT_ROOT).c \
			scanner.cc scanner.h y.output

dist-hook:
	rm -f $(distdir)/parser.cc $(distdir)/$(LEX_OUTPUT_ROOT).c \
	$(distdir)/scanner.cc $(distdir)/scanner.h $(distdir)/y.output

scanner.cc: parser.h

parser.h: parser.cc
