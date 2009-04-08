## Process this file with automake to produce Makefile.in

SUBDIRS=	be fe parser

DICELIBS=	$(top_builddir)/src/be/l4/libl4.a \
		$(top_builddir)/src/be/l4/v2/libv2.a \
		$(top_builddir)/src/be/l4/v2/amd64/libv2amd64.a \
		$(top_builddir)/src/be/l4/v2/ia32/libv2ia32.a \
		$(top_builddir)/src/be/l4/v4/libv4.a \
		$(top_builddir)/src/be/l4/v4/ia32/libv4ia32.a \
		$(top_builddir)/src/be/l4/fiasco/libfiasco.a \
		$(top_builddir)/src/be/l4/fiasco/amd64/libfiascoamd64.a \
		$(top_builddir)/src/be/sock/libsock.a \
		$(top_builddir)/src/be/libbe.a \
		$(top_builddir)/src/fe/libfe.a \
		$(top_builddir)/src/parser/idl/libidl.a \
		$(top_builddir)/src/parser/c-c++/libcparser.a \
		$(top_builddir)/src/parser/libparser.a

bin_PROGRAMS=	dice
dice_SOURCES = main.cpp Compiler.cpp Visitor.cpp version.cpp IncludeStatement.cpp Dependency.cpp Messages.cpp Object.cpp Location.cpp FactoryFactory.cpp

noinst_HEADERS = defines.h template.h Compiler.h Object.h Visitor.h ProgramOptions.h TypeSpec-Type.h Attribute-Type.h IncludeStatement.h Dependency.h Messages.h Error.h Location.h FactoryFactory.h

dice_DEPENDENCIES= $(DICELIBS)

LIBLTDL=@LIBLTDL@
dice_LDADD=	\
		-L$(top_builddir)/src/be/l4/v2 -lv2 \
		-L$(top_builddir)/src/be/l4/v2/amd64 -lv2amd64 \
		-L$(top_builddir)/src/be/l4/v2/ia32 -lv2ia32 \
		-L$(top_builddir)/src/be/l4/v4 -lv4 \
		-L$(top_builddir)/src/be/l4/v4/ia32 -lv4ia32 \
		-L$(top_builddir)/src/be/l4/fiasco -lfiasco \
		-L$(top_builddir)/src/be/l4/fiasco/amd64 -lfiascoamd64 \
		-L$(top_builddir)/src/be/l4 -ll4 \
		-L$(top_builddir)/src/be/sock -lsock \
		-L$(top_builddir)/src/be -lbe \
		-L$(top_builddir)/src/parser/idl -lidl \
		-L$(top_builddir)/src/parser/c-c++ -lcparser \
		-L$(top_builddir)/src/parser -lparser -lcparser \
		-L$(top_builddir)/src/fe -lfe \
		-lstdc++ $(LIBLTDL)

if HOST_DARWIN
# Darwin's _toupper and _tolower are macros, so using these as function
# pointers creates a linker error, which we avoid with some symbol mapping.
dice_LDFLAGS =
else
dice_LDFLAGS = -export-dynamic
endif

LTDLINCL=@LTDLINCL@
AM_CPPFLAGS=	-D__USER__="\"$(USER)@`hostname`\"" \
		$(LTDLINCL)

AM_CXXFLAGS = @DICE_CXXFLAGS@
ECHO = echo

CLEANFILES  = svnversion.h

wc:
	(find . \( -name "*.[chS]" -or -name "*.cpp" -or -name "*.ll" -or -name "*.yy" \) -and -not -name "*getopt*" ) | xargs wc -l

stats:
	cccc --help 2>&1|head -n 1 && rm -rf /tmp/cccc && mkdir /tmp/cccc && find . -name \*.cpp | xargs cccc --outdir=/tmp/cccc && install /tmp/cccc/* /mnt/oz/public_html/dice_stats/ && ln -f /mnt/oz/public_html/dice_stats/cccc.html /mnt/oz/public_html/dice_stats/index.html

.PHONY: svnversion.h.new

svnversion.h.new:
	if [ -d .svn ]; then \
	    ver=`svnversion -nc . | ${SED} -e 's/^[^:]*://;s/[A-Za-z]//'`; \
	    ${ECHO} "#define SVNVERSION \"svn r$$ver\"" > $@; \
	elif [ -s SNAPSHOT ]; then \
	    ver=`${SED} -e '/^Revision: */!d;s///;q' SNAPSHOT`; \
	    ${ECHO} "#define SVNVERSION \"svn r$$ver\"" > $@; \
	else \
	    touch $@; \
	fi

svnversion.h: svnversion.h.new
	if [ -e $@ ]; then \
	if [ 0 = `diff $@ $< | wc -l` ]; then \
	    rm $<; \
	else \
	    mv $< $@; \
	fi; \
	else \
	    mv $< $@; \
	fi

version.o: $(dice_SOURCES) $(DICELIBS) svnversion.h

install-exec-hook:
	cd $(DESTDIR)$(bindir) && \
	mv -f dice$(EXEEXT) dice-$(VERSION)$(EXEEXT) && \
	$(LN_S) dice-$(VERSION)$(EXEEXT) dice$(EXEEXT)

