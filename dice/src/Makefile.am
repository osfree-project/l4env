## Process this file with automake to produce Makefile.in

SUBDIRS=	be fe parser

DICELIBS=	$(top_builddir)/src/be/l4/libl4.a \
		$(top_builddir)/src/be/l4/v2/libv2.a \
		$(top_builddir)/src/be/l4/x0/libx0.a \
		$(top_builddir)/src/be/l4/x0/arm/libx0arm.a \
		$(top_builddir)/src/be/l4/x0/ia32/libx0ia32.a \
		$(top_builddir)/src/be/l4/x0adapt/libx0adapt.a \
		$(top_builddir)/src/be/l4/v4/libv4.a \
		$(top_builddir)/src/be/l4/v4/ia32/libv4ia32.a \
		$(top_builddir)/src/be/sock/libsock.a \
		$(top_builddir)/src/be/cdr/libcdr.a \
		$(top_builddir)/src/be/libbe.a \
		$(top_builddir)/src/fe/libfe.a \
		$(top_builddir)/src/parser/dce/libdce.a \
		$(top_builddir)/src/parser/corba/libcorba.a \
		$(top_builddir)/src/parser/gcc-c/libgcc_c.a \
		$(top_builddir)/src/parser/gcc-cxx/libgcc_cxx.a \
		$(top_builddir)/src/parser/inc/libinc.a

bin_PROGRAMS=	dice
dice_SOURCES = main.cpp Compiler.cpp CParser.cpp File.cpp Object.cpp version.cpp CCParser.cpp CCXXParser.cpp CDCEParser.cpp CCORBAParser.cpp CPreProcess.cpp IncludeStatement.cpp

noinst_HEADERS = defines.h Compiler.h CParser.h File.h Object.h ProgramOptions.h debug.h CCParser.h CCXXParser.h CDCEParser.h CCORBAParser.h CPreProcess.h TypeSpec-Type.h Attribute-Type.h IncludeStatement.h

dice_DEPENDENCIES= $(DICELIBS)

dice_LDADD=	-L$(top_builddir)/src/be/l4/v2 -lv2 \
		-L$(top_builddir)/src/be/l4/x0/arm -lx0arm \
		-L$(top_builddir)/src/be/l4/x0/ia32 -lx0ia32 \
		-L$(top_builddir)/src/be/l4/x0 -lx0 \
		-L$(top_builddir)/src/be/l4/x0adapt -lx0adapt \
		-L$(top_builddir)/src/be/l4/v4 -lv4 \
		-L$(top_builddir)/src/be/l4/v4/ia32 -lv4ia32 \
		-L$(top_builddir)/src/be/l4 -ll4 \
		-L$(top_builddir)/src/be/sock -lsock \
		-L$(top_builddir)/src/be/cdr -lcdr \
		-L$(top_builddir)/src/be -lbe \
		-L$(top_builddir)/src/parser/dce -ldce \
		-L$(top_builddir)/src/parser/corba -lcorba \
		-L$(top_builddir)/src/parser/inc -linc \
		-L$(top_builddir)/src/parser/gcc-c -lgcc_c \
		-L$(top_builddir)/src/parser/gcc-cxx -lgcc_cxx \
		-L$(top_builddir)/src/fe -lfe \
		-lstdc++

AM_CPPFLAGS=	-D__USER__="\"$(USER)@`hostname`\"" \
		-D__GCCVERSION__="\"`$(CXX) -v 2>&1 | tail -1`\"" \
		-D__CONFIGURECXX__="\"@CXX@\"" \
		-D__WHICHCXX__="\"`which gcc`\"" \
		-D__L4DIR__="\"`pwd`/${top_builddir}/@L4DIR@\"" \
		-D__DICEINCDIR__="\"${includedir}\""

wc:
	(find . \( -name "*.[chS]" -or -name "*.cpp" -or -name "*.ll" -or -name "*.yy" \) -and -not -name "*getopt*" ) | xargs wc -l

version.o: $(dice_SOURCES) $(DICELIBS)

