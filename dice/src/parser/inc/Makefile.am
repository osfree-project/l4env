## Process this file with automake to produce Makefile.in

noinst_LIBRARIES=	libinc.a
libinc_a_SOURCES=	scanner.ll

LFLAGS=         -Pinc -o$(LEX_OUTPUT_ROOT).c

CLEANFILES=             scanner.cc scanner.h 

dist-hook:
	rm -f i$(distdir)/scanner.cc $(distdir)/scanner.h
	
