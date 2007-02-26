## Process this file with automake to produce Makefile.in

SUBDIRS=	be fe parser

DICELIBS=	$(top_builddir)/src/be/l4/libl4.a \
		$(top_builddir)/src/be/l4/v2/libv2.a \
		$(top_builddir)/src/be/l4/x0/libx0.a \
		$(top_builddir)/src/be/l4/x0adapt/libx0adapt.a \
		$(top_builddir)/src/be/sock/libsock.a \
		$(top_builddir)/src/be/libbe.a \
		$(top_builddir)/src/fe/libfe.a \
		$(top_builddir)/src/parser/dce/libdce.a \
		$(top_builddir)/src/parser/corba/libcorba.a \
		$(top_builddir)/src/parser/inc/libinc.a

bin_PROGRAMS=	dice
dice_SOURCES = main.cpp Compiler.cpp CParser.cpp \
                File.cpp Vector.cpp Object.cpp \
		CString.cpp version.cpp

noinst_HEADERS = defines.h Compiler.h CParser.h \
		File.h Vector.h Object.h CString.h

dice_DEPENDENCIES= $(DICELIBS)

dice_LDADD=	-L$(top_builddir)/src/be/l4/v2 -lv2 \
		-L$(top_builddir)/src/be/l4/x0 -lx0 \
		-L$(top_builddir)/src/be/l4/x0adapt -lx0adapt \
		-L$(top_builddir)/src/be/l4 -ll4 \
		-L$(top_builddir)/src/be/sock -lsock \
		-L$(top_builddir)/src/be -lbe \
		-L$(top_builddir)/src/parser/dce -ldce \
		-L$(top_builddir)/src/parser/corba -lcorba \
		-L$(top_builddir)/src/parser/inc -linc \
		-L$(top_builddir)/src/fe -lfe

AM_CPPFLAGS=	-D__USER__="\"$(USER)@`hostname`\"" \
		-D__GCCVERSION__="\"`$(CXX) -v 2>&1 | tail -1`\"" \
		-D__CONFIGURECPP__="\"@CPP@\"" \
		-D__WHICHCPP__="\"`which cpp`\""

all-local: 
	@rm -f version.o

wc:
	(find . \( -name "*.[chS]" -or -name "*.cpp" -or -name "*.ll" -or -name "*.yy" \) -and -not -name "*getopt*" ) | xargs wc -l
