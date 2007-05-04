## Process this file with automake to produce Makefile.in

SUBDIRS=	be fe parser

DICELIBS=	$(top_builddir)/src/be/l4/libl4.a \
		$(top_builddir)/src/be/l4/v2/libv2.a \
		$(top_builddir)/src/be/l4/v2/amd64/libv2amd64.a \
		$(top_builddir)/src/be/l4/v2/ia32/libv2ia32.a \
		$(top_builddir)/src/be/l4/x0/libx0.a \
		$(top_builddir)/src/be/l4/v4/libv4.a \
		$(top_builddir)/src/be/l4/v4/ia32/libv4ia32.a \
		$(top_builddir)/src/be/sock/libsock.a \
		$(top_builddir)/src/be/libbe.a \
		$(top_builddir)/src/fe/libfe.a \
		$(top_builddir)/src/parser/dce/libdce.a \
		$(top_builddir)/src/parser/corba/libcorba.a \
		$(top_builddir)/src/parser/gcc-c/libgcc_c.a \
		$(top_builddir)/src/parser/gcc-cxx/libgcc_cxx.a \
		$(top_builddir)/src/parser/inc/libinc.a

bin_PROGRAMS=	dice
dice_SOURCES = main.cpp Compiler.cpp CParser.cpp File.cpp Object.cpp Visitor.cpp version.cpp CCParser.cpp CCXXParser.cpp CDCEParser.cpp CCORBAParser.cpp CPreProcess.cpp IncludeStatement.cpp Exception.cpp Dependency.cpp Messages.cpp

noinst_HEADERS = defines.h template.h Compiler.h CParser.h File.h Object.h Visitor.h ProgramOptions.h CCParser.h CCXXParser.h CDCEParser.h CCORBAParser.h CPreProcess.h TypeSpec-Type.h Attribute-Type.h IncludeStatement.h Exception.h Dependency.h Messages.h

dice_DEPENDENCIES= $(DICELIBS)

dice_LDADD=	-L$(top_builddir)/src/be/l4/v2 -lv2 \
		-L$(top_builddir)/src/be/l4/v2/amd64 -lv2amd64 \
		-L$(top_builddir)/src/be/l4/v2/ia32 -lv2ia32 \
		-L$(top_builddir)/src/be/l4/x0 -lx0 \
		-L$(top_builddir)/src/be/l4/v4 -lv4 \
		-L$(top_builddir)/src/be/l4/v4/ia32 -lv4ia32 \
		-L$(top_builddir)/src/be/l4 -ll4 \
		-L$(top_builddir)/src/be/sock -lsock \
		-L$(top_builddir)/src/be -lbe \
		-L$(top_builddir)/src/parser/dce -ldce \
		-L$(top_builddir)/src/parser/corba -lcorba \
		-L$(top_builddir)/src/parser/inc -linc \
		-L$(top_builddir)/src/parser/gcc-c -lgcc_c \
		-L$(top_builddir)/src/parser/gcc-cxx -lgcc_cxx \
		-L$(top_builddir)/src/fe -lfe \
		-lstdc++ -ldl

if HOST_DARWIN
# Darwin's _toupper and _tolower are macros, so using these as function
# pointers creates a linker error, which we avoid with some symbol mapping.
dice_LDFLAGS = -Wl,-i__toupper:_toupper -Wl,-i__tolower:_tolower
else
dice_LDFLAGS = -export-dynamic
endif

AM_CPPFLAGS=	-D__USER__="\"$(USER)@`hostname`\"" \
		-D__DICEINCDIR__="\"${includedir}\""

AM_CXXFLAGS = -Wunused -Wunused-parameter -Wall -W \
	      -Wcast-align -Wcast-qual -Wconversion -Wformat=2 \
	      -Wmissing-format-attribute -Wpointer-arith -Wshadow -Wwrite-strings

CLEANFILES  = svnversion.h

wc:
	(find . \( -name "*.[chS]" -or -name "*.cpp" -or -name "*.ll" -or -name "*.yy" \) -and -not -name "*getopt*" ) | xargs wc -l

stats:
	cccc --help 2>&1|head -n 1 && rm -rf /tmp/cccc && mkdir /tmp/cccc && find . -name \*.cpp | xargs cccc --outdir=/tmp/cccc && install /tmp/cccc/* /mnt/oz/public_html/dice_stats/ && ln -f /mnt/oz/public_html/dice_stats/cccc.html /mnt/oz/public_html/dice_stats/index.html

.PHONY: svnversion.h.new

svnversion.h.new:
	if [ -d .svn ]; then \
	    ver=`svnversion -nc . | sed -e 's/^[^:]*://;s/[A-Za-z]//'`; \
	    echo "#define SVNVERSION \"svn r$$ver\"" > $@; \
	elif [ -s SNAPSHOT ]; then \
	    ver=`sed -e '/^Revision: */!d;s///;q' SNAPSHOT`; \
	    echo "#define SVNVERSION \"svn r$$ver\"" > $@; \
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

